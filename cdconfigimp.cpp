/*
  Copyright (C) 2000 Rik Hemsley (rikkus) <rik@kde.org>
  Copyright (C) 2000, 2001 Michael Matz <matz@kde.org>
  Copyright (C) 2001 Carsten Duvenhorst <duvenhorst@m2.uni-hannover.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kconfig.h>
#include <kglobal.h>

#include <qfile.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C"
{
#include <cdda_interface.h>
#include <cdda_paranoia.h>

/* This is in support for the Mega Hack, if cdparanoia ever is fixed, or we
   use another ripping library we can remove this.*/ 
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,50)
typedef unsigned long long  __u64;
#endif
#include <linux/cdrom.h>
#include <sys/ioctl.h>
}

#include <kdebug.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <klocale.h>

#include "cdconfigimp.h"
#include "client.h"

//using namespace KIO;

#define MAX_IPC_SIZE (1024*32)

#define DEFAULT_CD_DEVICE "/dev/cdrom"

extern "C"
{
  int FixupTOC(cdrom_drive *d, int tracks);
}

int start_of_first_data_as_in_toc;
int hack_track;
/* Mega hack.  This function comes from libcdda_interface, and is called by
   it.  We need to override it, so we implement it ourself in the hope, that
   shared lib semantics make the calls in libcdda_interface to FixupTOC end
   up here, instead of it's own copy.  This usually works.
   You don't want to know the reason for this.  */
int FixupTOC(cdrom_drive *d, int tracks)
{
  int j;
  for (j = 0; j < tracks; j++) {
    if (d->disc_toc[j].dwStartSector < 0)
      d->disc_toc[j].dwStartSector = 0;
    if (j < tracks-1
        && d->disc_toc[j].dwStartSector > d->disc_toc[j+1].dwStartSector)
      d->disc_toc[j].dwStartSector = 0;
  }
  long last = d->disc_toc[0].dwStartSector;
  for (j = 1; j < tracks; j++) {
    if (d->disc_toc[j].dwStartSector < last)
      d->disc_toc[j].dwStartSector = last;
  }
  start_of_first_data_as_in_toc = -1;
  hack_track = -1;
  if (d->ioctl_fd != -1) {
    struct cdrom_multisession ms_str;
    ms_str.addr_format = CDROM_LBA;
    if (ioctl(d->ioctl_fd, CDROMMULTISESSION, &ms_str) == -1)
      return -1;
    if (ms_str.addr.lba > 100) {
      for (j = tracks-1; j >= 0; j--)
        if (j > 0 && !IS_AUDIO(d,j) && IS_AUDIO(d,j-1)) {
          if (d->disc_toc[j].dwStartSector > ms_str.addr.lba - 11400) {
            /* The next two code lines are the purpose of duplicating this
             * function, all others are an exact copy of paranoias FixupTOC().
             * The gory details: CD-Extra consist of N audio-tracks in the
             * first session and one data-track in the next session.  This
             * means, the first sector of the data track is not right behind
             * the last sector of the last audio track, so all length
             * calculation for that last audio track would be wrong.  For this
             * the start sector of the data track is adjusted (we don't need
             * the real start sector, as we don't rip that track anyway), so
             * that the last audio track end in the first session.  All well
             * and good so far.  BUT: The CDDB disc-id is based on the real
             * TOC entries so this adjustment would result in a wrong Disc-ID.
             * We can only solve this conflict, when we save the old
             * (toc-based) start sector of the data track.  Of course the
             * correct solution would be, to only adjust the _length_ of the
             * last audio track, not the start of the next track, but the
             * internal structures of cdparanoia are as they are, so the
             * length is only implicitely given.  Bloody sh*.  */
            start_of_first_data_as_in_toc = d->disc_toc[j].dwStartSector;
            hack_track = j + 1;
            d->disc_toc[j].dwStartSector = ms_str.addr.lba - 11400;
          }
          break;
        }
      return 1;
    }
  }
  return 0;
}

/* libcdda returns for cdda_disc_lastsector() the last sector of the last
   _audio_ track.  How broken.  For CDDB Disc-ID we need the real last sector
   to calculate the disc length.  */
long my_last_sector(cdrom_drive *drive)
{
  return cdda_track_lastsector(drive, drive->tracks);
}

enum Which_dir { Unknown = 0, Device, ByName, ByTrack, Title, Info, Root,
                 MP3, Vorbis };

class CdConfigImp::Private
{
  public:

    Private()
    {
      clear();
      discid = 0;
      based_on_cddb = false;
      s_byname = i18n("By Name");
      s_bytrack = i18n("By Track");
      s_track = i18n("Track %1");
      s_info = i18n("Information");
      s_mp3  = "MP3";
      s_vorbis = "Ogg Vorbis";
    }

    void clear()
    {
      which_dir = Unknown;
      req_track = -1;
    }

    QString path;
    int paranoiaLevel;
    bool useCDDB;
    unsigned int discid;
    int tracks;
    QString cd_title;
    QString cd_artist;
    QStringList titles;
    bool is_audio[100];
    bool based_on_cddb;
    QString s_byname;
    QString s_bytrack;
    QString s_track;
    QString s_info;
    QString s_mp3;
    QString s_vorbis;

    Which_dir which_dir;
    int req_track;
    QString fname;
};

CdConfigImp::CdConfigImp( QObject* parent, const char* name) : QObject(parent,name){
  d = new Private;
  //connect(getNow, SIGNAL(clicked()), this, SLOT(attemptToListAlbum()));
  loadSettings();

  //attemptToListAlbu/m();
  updating = false;

  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(timerDone()));
  if(constantlyScan)
    timer->start(3000, false);
  overrideCddb = false;
}

/**
 * Loads the settings
 */
void CdConfigImp::loadSettings(){
  KConfig &config = *KGlobal::config();
  config.setGroup("cdconfig");
  autoRip = config.readBoolEntry("autoRip", false);
  performCDDBauto = config.readBoolEntry("performCDDBauto", false);
  constantlyScan = config.readBoolEntry("constantlyScan", false); 
}

CdConfigImp::~CdConfigImp(){
  delete d;
}

/**
 *
 */
void CdConfigImp::cddbNow(){
  overrideCddb = true;
  timerDone();
  overrideCddb = false;
}

/**
 * check for a new cd and update
 */
void CdConfigImp::timerDone(){
  if(updating)
    return;
  updating = true;
  attemptToListAlbum();
  updating = false;
}

struct cdrom_drive * CdConfigImp::initRequest(const KURL & url) {

	// first get the parameters from the Kontrol Center Module
  getParameters();

	// then these parameters can be overruled by args in the URL
  parseArgs(url);


  struct cdrom_drive *drive = pickDrive();

  if (0 == drive){
    //error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return 0;
  }

  if (0 != cdda_open(drive)) {
    //error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.path());
    return 0;
  }

  if(updateCD(drive) == -1){
    return 0;
  }

  d->fname = url.filename(false);
  QString dname = url.directory(true, false);
  if (!dname.isEmpty() && dname[0] == '/')
    dname = dname.mid(1);

  /* A hack, for when konqi wants to list the directory audiocd:/Bla
     it really submits this URL, instead of audiocd:/Bla/ to us. We could
     send (in listDir) the UDS_NAME as "Bla/" for directories, but then
     konqi shows them as "Bla//" in the status line.  */
  if (dname.isEmpty() &&
      (d->fname == d->cd_title || d->fname == d->s_byname ||
       d->fname == d->s_bytrack || d->fname == d->s_info ||
       d->fname == d->s_mp3 || d->fname == d->s_vorbis || d->fname == "dev"))
    {
      dname = d->fname;
      d->fname = "";
    }

  if (dname.isEmpty())
    d->which_dir = Root;
  else if (dname == d->cd_title)
    d->which_dir = Title;
  else if (dname == d->s_byname)
    d->which_dir = ByName;
  else if (dname == d->s_bytrack)
    d->which_dir = ByTrack;
  else if (dname == d->s_info)
    d->which_dir = Info;
  else if (dname == d->s_mp3)
    d->which_dir = MP3;
  else if (dname == d->s_vorbis)
    d->which_dir = Vorbis;
  else if (dname.left(4) == "dev/")
    {
      d->which_dir = Device;
      dname = dname.mid(4);
    }
  else if (dname == "dev")
    {
      d->which_dir = Device;
      dname = "";
    }
  else
    d->which_dir = Unknown;

  d->req_track = -1;
  if (!d->fname.isEmpty())
    {
      QString n(d->fname);
      int pi = n.findRev('.');
      if (pi >= 0)
        n.truncate(pi);
      int i;
      for (i = 0; i < d->tracks; i++)
        if (d->titles[i] == n)
          break;
      if (i < d->tracks)
        d->req_track = i;
      else
        {
          /* Not found in title list.  Try hard to find a number in the
             string.  */
          unsigned int ui, j;
          ui = 0;
          while (ui < n.length())
            if (n[ui++].isDigit())
              break;
          for (j = ui; j < n.length(); j++)
            if (!n[j].isDigit())
              break;
          if (ui < n.length())
            {
              bool ok;
              /* The external representation counts from 1.  */
              d->req_track = n.mid(ui, j - i).toInt(&ok) - 1;
              if (!ok)
                d->req_track = -1;
            }
        }
    }
  if (d->req_track >= d->tracks)
    d->req_track = -1;

  kdDebug(60002) << "audiocd: dir=" << dname << " file=" << d->fname
    << " req_track=" << d->req_track << " which_dir=" << d->which_dir << endl;
  return drive;
}

unsigned int CdConfigImp::get_discid(struct cdrom_drive * drive) {
  unsigned int id = 0;
  for (int i = 1; i <= drive->tracks; i++)
    {
      unsigned int n = cdda_track_firstsector (drive, i) + 150;
      if (i == hack_track)
        n = start_of_first_data_as_in_toc + 150;
      n /= 75;
      while (n > 0)
        {
          id += n % 10;
          n /= 10;
        }
    }
  unsigned int l = (my_last_sector(drive));
  l -= cdda_disc_firstsector(drive);
  l /= 75;
  id = ((id % 255) << 24) | (l << 8) | drive->tracks;
  return id;
}

int CdConfigImp::updateCD(struct cdrom_drive * drive){
  unsigned int id = get_discid(drive);
  //BEN TODO
  //if (id == d->discid)
  //  return -1;
  
  d->discid = id;
  d->tracks = cdda_tracks(drive);
  d->cd_title = i18n("Unknown Album");
  d->cd_artist = i18n("Unknown Artist");
  d->titles.clear();
  KCDDB::TrackOffsetList qvl;

  for (int i = 0; i < d->tracks; i++)
    {
      d->is_audio[i] = cdda_track_audiop(drive, i + 1);
      if (i+1 != hack_track)
        qvl.append(cdda_track_firstsector(drive, i + 1) + 150);
      else
        qvl.append(start_of_first_data_as_in_toc + 150);
    }
  qvl.append(cdda_disc_firstsector(drive));
  qvl.append(my_last_sector(drive));

  if (performCDDBauto || overrideCddb)
  {
    KApplication::setOverrideCursor(Qt::waitCursor);

    KCDDB::Client c;

    KCDDB::CDDB::Result result = c.lookup(qvl);

    if (result == KCDDB::CDDB::Success)
    {
      d->based_on_cddb = true;
      KCDDB::CDInfo info = c.lookupResponse().first();

      d->cd_title = info.title;
      d->cd_artist = info.artist;

      KCDDB::TrackInfoList t = info.trackInfoList;
      for (unsigned i = 0; i < t.count(); i++)
      {
        QString n;
        n.sprintf("%02d ", i + 1);
        d->titles.append (n + t[i].title);
      }
      KApplication::restoreOverrideCursor();
      return 0;
    }
    KApplication::restoreOverrideCursor();
  }

  d->based_on_cddb = false;
  for (int i = 0; i < d->tracks; i++)
    {
      QString num;
      int ti = i + 1;
      QString s;
      num.sprintf("%02d", ti);
      if (cdda_track_audiop(drive, ti))
        s = d->s_track.arg(num);
      else
        s.sprintf("data%02d", ti);
      d->titles.append( s );
    }
  return 0;
}

/**
 * Attempt to list the files.
 */ 
void CdConfigImp::attemptToListAlbum(){
  KURL url = "/";
  struct cdrom_drive *drive = pickDrive();

  if (0 == drive) {
    emit(newAlbum(i18n("Unknown Artist"),i18n("Unknown Album"), 0, i18n("Other")));
    //error(KIO::ERR_DOES_NOT_EXIST, url.path());
    return;
  }

  if (0 != cdda_open(drive))
  {
    emit(newAlbum(i18n("Unknown Artist"),i18n("Unknown Album"), 0, i18n("Other")));
    //error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.path());
    return;
  }
  cdda_close(drive);

  drive = initRequest(url);
  if (!drive){
    return;
  }

  emit(newAlbum(d->cd_artist,d->cd_title, 0, i18n("Other")));
  for (int i = d->tracks; i > 0; i--){
    if (d->is_audio[i-1])
    {
      long size = CD_FRAMESIZE_RAW *
        ( cdda_track_lastsector(drive, i) - cdda_track_firstsector(drive, i));
      long length_seconds = size / 176400;
      emit(newSong(i,(d->titles[i-1]),length_seconds));
    }
  }
  if(autoRip)
    emit(ripAlbum());
  cdda_close(drive);
}

  struct cdrom_drive *
CdConfigImp::pickDrive()
{
  QCString path(QFile::encodeName(d->path));

  struct cdrom_drive * drive = 0;

  if (!path.isEmpty() && path != "/")
    drive = cdda_identify(path, CDDA_MESSAGE_PRINTIT, 0);

  else
  {
    drive = cdda_find_a_cdrom(CDDA_MESSAGE_PRINTIT, 0);

    if (0 == drive)
    {
      if (QFile(DEFAULT_CD_DEVICE).exists())
        drive = cdda_identify(DEFAULT_CD_DEVICE, CDDA_MESSAGE_PRINTIT, 0);
    }
  }

  if (0 == drive)
  {
    kdDebug(60002) << "Can't find an audio CD" << endl;
  }

  return drive;
}

  void
CdConfigImp::parseArgs(const KURL & url)
{
  bool old_use_cddb = d->useCDDB;

  d->clear();

  QString query(KURL::decode_string(url.query()));

  if (query.isEmpty() || query[0] != '?')
    return;

  query = query.mid(1); // Strip leading '?'.

  QStringList tokens(QStringList::split('&', query));

  for (QStringList::ConstIterator it(tokens.begin()); it != tokens.end(); ++it)
  {
    QString token(*it);

    int equalsPos(token.find('='));

    if (-1 == equalsPos)
      continue;

    QString attribute(token.left(equalsPos));
    QString value(token.mid(equalsPos + 1));

    if (attribute == "device")
    {
      d->path = value;
    }

    else if (attribute == "paranoia_level")
    {
      d->paranoiaLevel = value.toInt();
    }
    else if (attribute == "use_cddb")
    {
      d->useCDDB = (0 != value.toInt());
    }
  }

  /* We need to recheck the CD, if the user either enabled CDDB now. 
     We simply reset the saved discid, which forces a reread of CDDB
     information.  */

  if (old_use_cddb != d->useCDDB && d->useCDDB == true)
    d->discid = 0;

  kdDebug(60002) << "CDDB: use_cddb = " << d->useCDDB << endl;

}

void CdConfigImp::getParameters() {

  KConfig *config;
  config = new KConfig("kcmaudiocdrc");

  config->setGroup("CDDA");

  if (!config->readBoolEntry("autosearch",true)) {
     d->path = config->readEntry("device",DEFAULT_CD_DEVICE);
   }

  d->paranoiaLevel = 1; // enable paranoia error correction, but allow skipping

  if (config->readBoolEntry("disable_paranoia",false)) {
    d->paranoiaLevel = 0; // disable all paranoia error correction
  }

  if (config->readBoolEntry("never_skip",true)) {
    d->paranoiaLevel = 2;  // never skip on errors of the medium, should be default for high quality
  }

  config->setGroup("CDDB");

  d->useCDDB = config->readBoolEntry("enable_cddb",true);

  delete config;
}

#include "cdconfigimp.moc"

// cdconfigimp.cpp

