#include "tracksconfigimp.h"
#include <qlabel.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qheader.h> 
#include <qpixmap.h>
#include <kiconloader.h>
#include "job.h"
#include "id3tagdialog.h"
#include <qlineedit.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <kmessagebox.h>
#include <klocale.h>

#define HEADER_TRACK 0
#define HEADER_NAME 3
#define HEADER_LENGTH 2
#define HEADER_RIP 1

/**
 * Constructor, connect up slots and signals.
 */
TracksConfigImp::TracksConfigImp( QWidget* parent, const char* name):TracksConfig(parent,name), album(""), group(""), genre(""), year(-1),allOn(false){
  connect(ripSelectedTracks, SIGNAL(clicked()), this, SLOT(startSession()));
  connect(editTag, SIGNAL(clicked()), this, SLOT(editInformation()));
  connect(trackListing, SIGNAL(clicked( QListViewItem * )), this, SLOT(selectTrack(QListViewItem*))); 
  connect(trackListing->header(), SIGNAL(clicked(int)), this, SLOT(headerClicked(int)));
  connect(refreshList, SIGNAL(clicked()), this, SIGNAL(refreshCd()));
  trackListing->setSorting(-1, false);
}

/**
 * Bring up the dialog to edit the information about this album.
 * If there is not currently selected track return.
 * If ok is pressed then store the information and update track name.
 */
void TracksConfigImp::editInformation(){
  QListViewItem * currentItem = trackListing->currentItem();
  if( currentItem == 0 ){
    KMessageBox::sorry(this, i18n("Please select a track"), i18n("No track selected"));
    return;
  }

  // Create dialog.
  Id3TagDialog dialog(this, "Album info editor dialog", true);
  dialog.artist->setText(group);
  dialog.album->setText(album);
  dialog.year->setValue(year);
  dialog.trackLabel->setText(QString("Track %1").arg(currentItem->text(HEADER_TRACK)));
  dialog.title->setText(currentItem->text(HEADER_NAME));
  int totalGenres = dialog.genre->count();
  if(genre == "")
    genre = "Other";
  for(int i = 0; i < totalGenres; i++){
    if(dialog.genre->text(i) == genre){
      dialog.genre->setCurrentItem(i);
      break;
    }
  }
 
  // Show dialog and save results.
  bool okClicked = dialog.exec();
  if(okClicked){
    group = dialog.artist->text();
    album = dialog.album->text();
    year = dialog.year->value();
    currentItem->setText(HEADER_NAME, dialog.title->text());
    genre = dialog.genre->currentText();
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
  QString list = "";
  if( genre == "" )
    list += "Genre";
  
  if( year == 0 ){
    if(list != "")
      list += ", ";
    list += "Year";
  }
  if( group == "No Artist"){
    if(list != "")
      list += ", ";
    list += "Artist";
  }
  if( album == "No Album"){
    if(list != "")
      list += ", ";
    list += "Album";
  }
  if( list != ""){
    int r = KMessageBox:: questionYesNo(this, i18n(QString("Part of the album is not set: %1. Would you like to continue anyway?").arg(list).latin1()), i18n("Album Information incomplete"));
    if( r == KMessageBox::No )
      return;
  }
  QListViewItem * currentItem = trackListing->firstChild();
  Job *lastJob = NULL;
  while( currentItem != 0 ){
    if(currentItem->pixmap(HEADER_RIP) != NULL ){
      Job *newJob = new Job();
      newJob->album = album;
      newJob->genre = genre;
      newJob->group = group;
      newJob->song = currentItem->text(HEADER_NAME);
      newJob->track = currentItem->text(HEADER_TRACK).toInt();
      newJob->year = year;
      lastJob = newJob;
      emit( ripTrack(newJob) ); 
    }
    currentItem = currentItem->nextSibling();
  }
  if(lastJob)
    lastJob->lastSongInAlbum = true;
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
  QString songLength = QString("%1:%2%3").arg(length/60).arg((length % 60)/10).arg((length % 60)%10);
  QListViewItem * newItem = new QListViewItem(trackListing, QString("%1").arg(track), "", songLength, song);
  trackListing->setCurrentItem(trackListing->firstChild());
}

#include "tracksconfigimp.moc"

// trackconfigimp.cpp

