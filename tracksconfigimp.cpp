#include "tracksconfigimp.h"
#include "job.h"
#include "id3tagdialog.h"
#include <qlabel.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qheader.h> 
#include <qlineedit.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qpixmap.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kurl.h>
#include <qregexp.h>

#define HEADER_TRACK 1
#define HEADER_NAME 3
#define HEADER_LENGTH 2
#define HEADER_RIP 0

/**
 * Constructor, connect up slots and signals.
 */
TracksConfigImp::TracksConfigImp( QWidget* parent, const char* name):TracksConfig(parent,name), album(""), group(""), genre(""), year(-1),allOn(false){
  connect(ripSelectedTracks, SIGNAL(clicked()), this, SLOT(startSession()));
  connect(editTag, SIGNAL(clicked()), this, SLOT(editInformation()));
  connect(trackListing, SIGNAL(clicked( QListViewItem * )), this, SLOT(selectTrack(QListViewItem*))); 
  connect(trackListing, SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(editInformation()));
  connect(trackListing->header(), SIGNAL(clicked(int)), this, SLOT(headerClicked(int)));
  connect(refreshList, SIGNAL(clicked()), this, SIGNAL(refreshCd()));
  connect(selectAllTracksButton, SIGNAL(clicked()), this, SLOT(selectAllTracks()));
  trackListing->setSorting(-1, false);
  genres.insert(i18n("A Cappella"), "A Cappella");
  genres.insert(i18n("Acid Jazz"), "Acid Jazz");
  genres.insert(i18n("Acid Punk"), "Acid Punk");
  genres.insert(i18n("Acid"), "Acid");
  genres.insert(i18n("Acoustic"), "Acoustic");
  genres.insert(i18n("Alternative"), "Alternative");
  genres.insert(i18n("Alt. Rock"), "Alt. Rock");
  genres.insert(i18n("Ambient"), "Ambient");
  genres.insert(i18n("Anime"), "Anime");
  genres.insert(i18n("Avantgarde"), "Avantgarde");
  genres.insert(i18n("Ballad"), "Ballad");
  genres.insert(i18n("Bass"), "Bass");
  genres.insert(i18n("Beat"), "Beat");
  genres.insert(i18n("Bebop"), "Bebop");
  genres.insert(i18n("Big Band"), "Big Band");
  genres.insert(i18n("Black Metal"), "Black Metal");
  genres.insert(i18n("Bluegrass"), "Bluegrass");
  genres.insert(i18n("Blues"), "Blues");
  genres.insert(i18n("Booty Bass"), "Booty Bass");
  genres.insert(i18n("BritPop"), "BritPop");
  genres.insert(i18n("Cabaret"), "Cabaret");
  genres.insert(i18n("Celtic"), "Celtic");
  genres.insert(i18n("Chamber Music"), "Chamber Music");
  genres.insert(i18n("Chanson"), "Chanson");
  genres.insert(i18n("Chorus"), "Chorus");
  genres.insert(i18n("Christian Gangsta Rap"), "Christian Gangsta Rap");
  genres.insert(i18n("Christian Rap"), "Christian Rap");
  genres.insert(i18n("Christian Rock"), "Christian Rock");
  genres.insert(i18n("Classical"), "Classical");
  genres.insert(i18n("Classic Rock"), "Classic Rock");
  genres.insert(i18n("Club-house"), "Club-house");
  genres.insert(i18n("Club"), "Club");
  genres.insert(i18n("Comedy"), "Comedy");
  genres.insert(i18n("Contemporary Christian"), "Contemporary Christian");
  genres.insert(i18n("Country"), "Country");
  genres.insert(i18n("Crossover"), "Crossover");
  genres.insert(i18n("Cult"), "Cult");
  genres.insert(i18n("Dance Hall"), "Dance Hall");
  genres.insert(i18n("Dance"), "Dance");
  genres.insert(i18n("Darkwave"), "Darkwave");
  genres.insert(i18n("Death Metal"), "Death Metal");
  genres.insert(i18n("Disco"), "Disco");
  genres.insert(i18n("Dream"), "Dream");
  genres.insert(i18n("Drum & Bass"), "Drum & Bass");
  genres.insert(i18n("Drum Solo"), "Drum Solo");
  genres.insert(i18n("Duet"), "Duet");
  genres.insert(i18n("Easy Listening"), "Easy Listening");
  genres.insert(i18n("Electronic"), "Electronic");
  genres.insert(i18n("Ethnic"), "Ethnic");
  genres.insert(i18n("Eurodance"), "Eurodance");
  genres.insert(i18n("Euro-House"), "Euro-House");
  genres.insert(i18n("Euro-Techno"), "Euro-Techno");
  genres.insert(i18n("Fast-Fusion"), "Fast-Fusion");
  genres.insert(i18n("Folklore"), "Folklore");
  genres.insert(i18n("Folk/Rock"), "Folk/Rock");
  genres.insert(i18n("Folk"), "Folk");
  genres.insert(i18n("Freestyle"), "Freestyle");
  genres.insert(i18n("Funk"), "Funk");
  genres.insert(i18n("Fusion"), "Fusion");
  genres.insert(i18n("Game"), "Game");
  genres.insert(i18n("Gangsta Rap"), "Gangsta Rap");
  genres.insert(i18n("Goa"), "Goa");
  genres.insert(i18n("Gospel"), "Gospel");
  genres.insert(i18n("Gothic Rock"), "Gothic Rock");
  genres.insert(i18n("Gothic"), "Gothic");
  genres.insert(i18n("Grunge"), "Grunge");
  genres.insert(i18n("Hardcore"), "Hardcore");
  genres.insert(i18n("Hard Rock"), "Hard Rock");
  genres.insert(i18n("Heavy Metal"), "Heavy Metal");
  genres.insert(i18n("Hip-Hop"), "Hip-Hop");
  genres.insert(i18n("House"), "House");
  genres.insert(i18n("Humor"), "Humor");
  genres.insert(i18n("Indie"), "Indie");
  genres.insert(i18n("Industrial"), "Industrial");
  genres.insert(i18n("Instrumental Pop"), "Instrumental Pop");
  genres.insert(i18n("Instrumental Rock"), "Instrumental Rock");
  genres.insert(i18n("Instrumental"), "Instrumental");
  genres.insert(i18n("Jazz+Funk"), "Jazz+Funk");
  genres.insert(i18n("Jazz"), "Jazz");
  genres.insert(i18n("JPop"), "JPop");
  genres.insert(i18n("Jungle"), "Jungle");
  genres.insert(i18n("Latin"), "Latin");
  genres.insert(i18n("Lo-Fi"), "Lo-Fi");
  genres.insert(i18n("Meditative"), "Meditative");
  genres.insert(i18n("Merengue"), "Merengue");
  genres.insert(i18n("Metal"), "Metal");
  genres.insert(i18n("Musical"), "Musical");
  genres.insert(i18n("National Folk"), "National Folk");
  genres.insert(i18n("Native American"), "Native American");
  genres.insert(i18n("Negerpunk"), "Negerpunk");
  genres.insert(i18n("New Age"), "New Age");
  genres.insert(i18n("New Wave"), "New Wave");
  genres.insert(i18n("Noise"), "Noise");
  genres.insert(i18n("Oldies"), "Oldies");
  genres.insert(i18n("Opera"), "Opera");
  genres.insert(i18n("Other"), "Other");
  genres.insert(i18n("Polka"), "Polka");
  genres.insert(i18n("Polsk Punk"), "Polsk Punk");
  genres.insert(i18n("Pop-Funk"), "Pop-Funk");
  genres.insert(i18n("Pop/Funk"), "Pop/Funk");
  genres.insert(i18n("Pop"), "Pop");
  genres.insert(i18n("Porn Groove"), "Porn Groove");
  genres.insert(i18n("Power Ballad"), "Power Ballad");
  genres.insert(i18n("Pranks"), "Pranks");
  genres.insert(i18n("Primus"), "Primus");
  genres.insert(i18n("Progressive Rock"), "Progressive Rock");
  genres.insert(i18n("Psychedelic Rock"), "Psychedelic Rock");
  genres.insert(i18n("Psychedelic"), "Psychedelic");
  genres.insert(i18n("Punk Rock"), "Punk Rock");
  genres.insert(i18n("Punk"), "Punk");
  genres.insert(i18n("R&B"), "R&B");
  genres.insert(i18n("Rap"), "Rap");
  genres.insert(i18n("Rave"), "Rave");
  genres.insert(i18n("Reggae"), "Reggae");
  genres.insert(i18n("Retro"), "Retro");
  genres.insert(i18n("Revival"), "Revival");
  genres.insert(i18n("Rhythmic Soul"), "Rhythmic Soul");
  genres.insert(i18n("Rock & Roll"), "Rock & Roll");
  genres.insert(i18n("Rock"), "Rock");
  genres.insert(i18n("Salsa"), "Salsa");
  genres.insert(i18n("Samba"), "Samba");
  genres.insert(i18n("Satire"), "Satire");
  genres.insert(i18n("Showtunes"), "Showtunes");
  genres.insert(i18n("Ska"), "Ska");
  genres.insert(i18n("Slow Jam"), "Slow Jam");
  genres.insert(i18n("Slow Rock"), "Slow Rock");
  genres.insert(i18n("Sonata"), "Sonata");
  genres.insert(i18n("Soul"), "Soul");
  genres.insert(i18n("Sound Clip"), "Sound Clip");
  genres.insert(i18n("Soundtrack"), "Soundtrack");
  genres.insert(i18n("Southern Rock"), "Southern Rock");
  genres.insert(i18n("Space"), "Space");
  genres.insert(i18n("Speech"), "Speech");
  genres.insert(i18n("Swing"), "Swing");
  genres.insert(i18n("Symphonic Rock"), "Symphonic Rock");
  genres.insert(i18n("Symphony"), "Symphony");
  genres.insert(i18n("Synthpop"), "Synthpop");
  genres.insert(i18n("Tango"), "Tango");
  genres.insert(i18n("Techno-Industrial"), "Techno-Industrial");
  genres.insert(i18n("Techno"), "Techno");
  genres.insert(i18n("Terror"), "Terror");
  genres.insert(i18n("Thrash Metal"), "Thrash Metal");
  genres.insert(i18n("Top 40"), "Top 40");
  genres.insert(i18n("Trailer"), "Trailer");
  genres.insert(i18n("Trance"), "Trance");
  genres.insert(i18n("Tribal"), "Tribal");
  genres.insert(i18n("Trip-Hop"), "Trip-Hop");
  genres.insert(i18n("Vocal"), "Vocal");
}

/**
 * Bring up the dialog to edit the information about this album.
 * If there is not currently selected track return.
 * If ok is pressed then store the information and update track name.
 */
void TracksConfigImp::editInformation(){
  QListViewItem * currentItem = trackListing->currentItem();
  if( currentItem == 0 ){
    KMessageBox::sorry(this, i18n("Please select a track."), i18n("No Track Selected"));
    return;
  }

  // Create dialog.
  Id3TagDialog dialog(this, "Album info editor dialog", true);
  dialog.artist->setText(group);
  dialog.album->setText(album);
  dialog.year->setValue(year);
  dialog.trackLabel->setText(i18n("Track %1").arg(currentItem->text(HEADER_TRACK)));
  dialog.title->setText(currentItem->text(HEADER_NAME));
  dialog.genre->insertStringList(genres.keys());
  int totalGenres = dialog.genre->count();
  if(genre == "")
    genre = i18n("Other");
  
  for(int i = 0; i < totalGenres; i++){
    if(dialog.genre->text(i) == genre){
      dialog.genre->setCurrentItem(i);
      break;
    }
  }

  // set focus to track title
  dialog.title->setFocus();
  dialog.title->selectAll();

  // Show dialog and save results.
  bool okClicked = dialog.exec();
  if(okClicked){
    group = dialog.artist->text();
    album = dialog.album->text();
    year = dialog.year->value();
    currentItem->setText(HEADER_NAME, dialog.title->text());
    genre = dialog.genre->currentText();

    QString newTitle = QString("%1 - %2").arg(group).arg(album);
    if(albumName->text() != newTitle)
      albumName->setText(newTitle);
  }
}

/**
 * Helper function.  Checks all tracks and then calls startSession to rip them all.
 */
void TracksConfigImp::ripWholeAlbum(){
  allOn = false;
  headerClicked(3);
  startSession();
}

/**
 * Start of the "ripping session" by emiting signals to rip the selected tracks.
 * If any album information is not set, notify the user first.
 */
void TracksConfigImp::startSession(){
  if(trackListing->childCount() == 0){
    KMessageBox:: sorry(this, i18n("No tracks are selected to rip. Please select at least 1 track before ripping."), i18n("No Tracks Selected"));
    return;
  }

  QString list = "";
  if( genre == "" )
    list += "Genre";
  
  if( year == 0 ){
    if(list != "")
      list += ", ";
    list += "Year";
  }
  if( group == "Unknown Artist"){
    if(list != "")
      list += ", ";
    list += "Artist";
  }
  if( album == "Unknown Album"){
    if(list != "")
      list += ", ";
    list += "Album";
  }
  if( list != ""){
    int r = KMessageBox::questionYesNo(this, i18n("Part of the album is not set: %1.\n (To change album information click the \"Edit Information\" button.)\n Would you like to rip the selected tracks anyway?").arg(list), i18n("Album Information Incomplete"));
    if( r == KMessageBox::No )
      return;
  }
  QListViewItem * currentItem = trackListing->firstChild();
  Job *lastJob = NULL;
  int counter = 0;
  while( currentItem != 0 ){
    if(currentItem->pixmap(HEADER_RIP) != NULL ){
      Job *newJob = new Job();
      newJob->album = album;
      newJob->genre = genres[genre];
      newJob->group = group;
      newJob->song = currentItem->text(HEADER_NAME);
      newJob->track = currentItem->text(HEADER_TRACK).toInt();
      newJob->year = year;
      lastJob = newJob;
      emit( ripTrack(newJob) ); 
    counter++;
    }
    currentItem = currentItem->nextSibling();
  }
  if(lastJob)
    lastJob->lastSongInAlbum = true;

  if(counter == 0){
    KMessageBox:: sorry(this, i18n("No tracks are selected to rip. Please select at least 1 track before ripping."), i18n("No Tracks Selected"));
    return;
  }

  KMessageBox::information(this,
  i18n("%1 Job(s) have been started.  You can watch their progress in the jobs section.").arg(counter),
 i18n("Jobs have started"), i18n("Jobs have started"));
}

/**
 * The header was clicked so turn all of the tracks on/off
 */
void TracksConfigImp::headerClicked(int){
  allOn = !allOn;
 
  // If the user manually turned them all on or off make sure we don't do the same. 
  int totalSelectedSongs = 0;
  QListViewItem * currentItem = trackListing->firstChild();
  while( currentItem != 0 ){
    if(currentItem->pixmap(HEADER_RIP) != NULL )
      totalSelectedSongs++;
    currentItem = currentItem->nextSibling();
  }
  if(totalSelectedSongs == 0)
    allOn = true;
  if(totalSelectedSongs == trackListing->childCount())
    allOn = false;

  // Turn them all on or off.
  currentItem = trackListing->firstChild();
  while( currentItem != 0 ){
    if(allOn)
      currentItem->setPixmap(HEADER_RIP, SmallIcon("check"));
    else{
      QPixmap emptyPixmap;
      currentItem->setPixmap(HEADER_RIP, emptyPixmap);
    }
    currentItem = currentItem->nextSibling();
  }
}

/**
 * Selects and unselects the tracks.
 * @param currentItem the track to swich the selection choice.
 */
void TracksConfigImp::selectTrack(QListViewItem *currentItem){
  if(!currentItem)
    return;
  if(currentItem->pixmap(HEADER_RIP) != NULL){
    QPixmap empty;
    currentItem->setPixmap(HEADER_RIP, empty);
  }
  else
    currentItem->setPixmap(HEADER_RIP, SmallIcon("check"));
}

/**
 * Turn on all of the tracks.
 */
void TracksConfigImp::selectAllTracks(){
  QListViewItem *currentItem = trackListing->firstChild();
  while( currentItem != 0 ){
    currentItem->setPixmap(HEADER_RIP, SmallIcon("check"));
    currentItem = currentItem->nextSibling();
  }
}

/**
 * Set the current stats for the new album being displayed.
 */
void TracksConfigImp::newAlbum(QString newGroup, QString newAlbum, int newYear, QString newGenre){
  albumName->setText(QString("%1 - %2").arg(newGroup).arg(newAlbum));
  trackListing->clear();
  album = newAlbum;
  group = newGroup;
  year = newYear;
  genre = newGenre;
}

/**
 * There is a new song for this album.  Add it to the list of songs.  Set the current selected
 * song to the first one.
 * @param track the track number for the song.
 * @param song the name of the song.
 */
void TracksConfigImp::newSong(int track, QString song, int length){
  song = song.mid(song.find(' ',0)+1, song.length());
  song = KURL::decode_string(song);
  song.replace(QRegExp("/"), "-");
  QString songLength = QString("%1:%2%3").arg(length/60).arg((length % 60)/10).arg((length % 60)%10);
  QListViewItem * newItem = new QListViewItem(trackListing, "", QString("%1").arg(track), songLength, song);
  trackListing->setCurrentItem(trackListing->firstChild());
}

#include "tracksconfigimp.moc"

// trackconfigimp.cpp

