#include "encoderconfigimp.h"
#include "wizard.h"

#include <qapplication.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdir.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kurl.h>
#include <kconfig.h>
#include <kglobal.h>

#define ENCODER_EXE_STRING "encoderExe_"
#define ENCODER_ARGS_STRING "encoderCommandLine_"
#define ENCODER_EXTENSION_STRING "encoderExtension_"
#define ENCODER_PERCENTLENGTH_STRING "encoderPercentLength_"

/**
 * A helper function to replace %X with the stuff in the album.
 * if slash it tru then put "" around the %X
 */
void EncoderConfigImp::replaceSpecialChars(QString &string, Job * job, bool slash){
  if(slash == true){
    string.replace(QRegExp("%album"), QString("\"%1\"").arg(job->album));
    string.replace(QRegExp("%genre"), QString("\"%1\"").arg(job->genre));
    string.replace(QRegExp("%artist"), QString("\"%1\"").arg(job->group));
    string.replace(QRegExp("%year"), QString("\"%1\"").arg(job->year));
    string.replace(QRegExp("%song"), QString("\"%1\"").arg(job->song));
    string.replace(QRegExp("%extension"), QString("\"%1\"").arg(encoderExtension->text()));
    if( job->track < 10 )
      string.replace(QRegExp("%track"), QString("\"0%1\"").arg(job->track));
    else
      string.replace(QRegExp("%track"), QString("\"%1\"").arg(job->track));
    return;
  }
  else{
    string.replace(QRegExp("%album"), QString("%1").arg(job->album));
    string.replace(QRegExp("%genre"), QString("%1").arg(job->genre));
    string.replace(QRegExp("%artist"), QString("%1").arg(job->group));
    string.replace(QRegExp("%year"), QString("%1").arg(job->year));
    string.replace(QRegExp("%song"), QString("%1").arg(job->song));
    string.replace(QRegExp("%extension"), QString("%1").arg(encoderExtension->text()));
    if( job->track < 10 )
      string.replace(QRegExp("%track"), QString("0%1").arg(job->track));
    else
      string.replace(QRegExp("%track"), QString("%1").arg(job->track));
    return;
  }
}

/**
 * Constructor, load settings.  Set the up the pull down menu with the correct item.
 */
EncoderConfigImp::EncoderConfigImp( QWidget* parent, const char* name):EncoderConfig(parent,name), encodersPercentStringLength(2), oldEncoderSelection(-1){
  connect(encoder, SIGNAL(activated(int)), this, SLOT(loadEncoderConfig(int)));
  connect(playlistWizardButton, SIGNAL(clicked()), this, SLOT(playlistWizard()));
  connect(encoderWizardButton, SIGNAL(clicked()), this, SLOT(encoderWizard()));
  
  KConfig &config = *KGlobal::config();
  config.setGroup("encoderconfig");
  saveWav->setChecked(config.readBoolEntry("saveWav", false));
  numberOfCpus->setValue(config.readNumEntry("numberOfCpus", 1));
  fileFormat->setText(config.readEntry("fileFormat", "~/%extension/%artist/%album/%artist - %song.%extension"));
  createPlaylistCheckBox->setChecked(config.readBoolEntry("createPlaylist", false));
  playlistFileFormat->setText(config.readEntry("playlistFileFormat", "~/%extension/%artist/%album/%artist - %album.m3u"));
  useRelitivePath->setChecked(config.readBoolEntry("useRelitivePath", false));
  int totalNumberOfEncoders = config.readNumEntry("numberOfEncoders",0);
 
  /***
   * The Encoders can be entirly loaded and are not hard coded.  You can add remove them on the fly
   * using the configure file, but a set of default ones are include here.
   *
   * If you would like to add a default value to here contact ben@meyerhome.net with apropriate values
   *
   * Encoder name - name of the encoder
   * Arguments - Command line string to encoder a file using that encoder
   * Extension - File extension that is generated.
   * Percent output length.  99.00% == 4, 99.9% == 3, 99% == 2
   */  
  if(totalNumberOfEncoders==0){
    config.writeEntry( ENCODER_EXE_STRING "0", "Lame");
    config.writeEntry( ENCODER_ARGS_STRING "0", "lame --r3mix --tt %song --ta %artist --tl %album --ty %year --tn %track --tg %genre %f %o");
    config.writeEntry( ENCODER_EXTENSION_STRING "0", "mp3");
    config.writeEntry( ENCODER_PERCENTLENGTH_STRING "0", 2);
    
    config.writeEntry( ENCODER_EXE_STRING "2", "Leave As Wav");
    config.writeEntry( ENCODER_ARGS_STRING "2", "mv %f %o");
    config.writeEntry( ENCODER_EXTENSION_STRING "2", "wav");
    config.writeEntry( ENCODER_PERCENTLENGTH_STRING "2", 2);
    
    config.writeEntry( ENCODER_EXE_STRING "1", "OggEnc");
    config.writeEntry( ENCODER_ARGS_STRING "1", "oggenc -o %o -a %artist -l %album -t %song -N %track %f");
    config.writeEntry( ENCODER_EXTENSION_STRING "1", "ogg");
    config.writeEntry( ENCODER_PERCENTLENGTH_STRING "1", 4);
    
    config.writeEntry( ENCODER_EXE_STRING "3", "Other");
    config.writeEntry( ENCODER_ARGS_STRING "3", "");
    config.writeEntry( ENCODER_EXTENSION_STRING "3", "");
    config.writeEntry( ENCODER_PERCENTLENGTH_STRING "3", 2);
    
    totalNumberOfEncoders = 4;
    config.writeEntry("numberOfEncoders",4);
  }

  for(int i=0; i < totalNumberOfEncoders; i++){
    encoder->insertItem(config.readEntry(QString(ENCODER_EXE_STRING "%1").arg(i),""),i);
  }
  
  // Set the current item and settings.
  int currentItem = config.readNumEntry("encoderCurrentItem",0);
  encoder->setCurrentItem(currentItem);
  loadEncoderConfig(encoder->currentItem());
}

/**
 * Deconstructor, remove pending jobs, remove current jobs, save settings.
 */
EncoderConfigImp::~EncoderConfigImp(){
  pendingJobs.clear();

  QMap<KShellProcess*, Job*>::Iterator it;
  for( it = jobs.begin(); it != jobs.end(); ++it ){
    
	  KShellProcess *process = it.key();
    Job *job = jobs[it.key()];
    threads.remove(process);
    process->kill();
    QFile::remove(job->newLocation);
    delete job;
    delete process;
  }
  jobs.clear();

  KConfig &config = *KGlobal::config();
  config.setGroup("encoderconfig");
  config.writeEntry("encoderCurrentItem", encoder->currentItem());
  config.writeEntry("saveWav", saveWav->isChecked());
  config.writeEntry("numberOfCpus", numberOfCpus->value());
  config.writeEntry("fileFormat", fileFormat->text());
  config.writeEntry("createPlaylist", createPlaylistCheckBox->isChecked());
  config.writeEntry("playlistFileFormat", playlistFileFormat->text());
  config.writeEntry("useRelitivePath", useRelitivePath->isChecked());

  int index = encoder->currentItem();
  config.writeEntry(QString(ENCODER_ARGS_STRING "%1").arg(index), encoderCommandLine->text());
  config.writeEntry(QString(ENCODER_EXTENSION_STRING "%1").arg(index), encoderExtension->text());
}

/**
 * Load the settings for this encoder.
 * @param index the selected item in the drop down menu.
 */
void EncoderConfigImp::loadEncoderConfig(int index){
  KConfig &config = *KGlobal::config();
  config.setGroup("encoderconfig");
  
  // You have to save the old one first.
  if(oldEncoderSelection != -1){
    config.writeEntry(QString(ENCODER_ARGS_STRING "%1").arg(oldEncoderSelection), encoderCommandLine->text());
    config.writeEntry(QString(ENCODER_EXTENSION_STRING "%1").arg(oldEncoderSelection), encoderExtension->text());
  }
  oldEncoderSelection = index;

  // Now you can load the new settings.
  encoderCommandLine->setText(config.readEntry(QString(ENCODER_ARGS_STRING "%1").arg(index), ""));
  encoderExtension->setText(config.readEntry(QString(ENCODER_EXTENSION_STRING "%1").arg(index), ""));
  encodersPercentStringLength = config.readNumEntry( QString(ENCODER_PERCENTLENGTH_STRING "%1").arg(index),2);
}

/**
 * Stop this job with the matchin id.
 * @param id the id number of the job to stop.
 */
void EncoderConfigImp::removeJob(int id){
  QMap<KShellProcess*, Job*>::Iterator it;
  for( it = jobs.begin(); it != jobs.end(); ++it ){
    if(it.data()->id == id){
      KShellProcess *process = it.key();
      Job *job = jobs[it.key()];
      threads.remove(process);
      process->kill();
      jobs.remove(process);
      delete job;
      delete process;
      break;
    }
  }
  Job *job = pendingJobs.first();
  while(job){
    if(job->id == id)
      break;
    job = pendingJobs.next();
  }
  if(job){
    pendingJobs.remove(job);
    delete job;
  }
}

/**
 * Adds job to the que of jobs to encode.
 * @param job the job to encode.
 */
void EncoderConfigImp::encodeWav(Job *job){
  emit(addJob(job, i18n("Encoding: %1 - %2").arg(job->group).arg(job->song)));
  pendingJobs.append(job);
  tendToNewJobs();
}

/**
 * See if there are are new jobs to attend too.  If we are all loaded up
 * then just loop back in 5 seconds and check agian.
 */
void EncoderConfigImp::tendToNewJobs(){
  while(threads.count() >= (uint)numberOfCpus->value()){
    QTimer::singleShot( threads.count()*2*1000, this, SLOT(tendToNewJobs()));
    return;
  }
  if(pendingJobs.count() == 0)
    return;
 
  Job *job = pendingJobs.first();
  pendingJobs.remove(job);
  job->jobType = encoder->currentItem();

  QString desiredFile = fileFormat->text();
  replaceSpecialChars(desiredFile, job, false);
  if(desiredFile[0] == '~'){
    desiredFile.replace(0,1, QDir::homeDirPath());
  }
  int lastSlash = desiredFile.findRev('/',-1);
  if( lastSlash == -1 || !(KStandardDirs::makeDir( desiredFile.mid(0,lastSlash)))){
   qDebug("Can not place file, unable to make directories");
    return;
  }

  job->newLocation = desiredFile;
  
  QString command = encoderCommandLine->text();
  replaceSpecialChars(command, job, true);
  command.replace(QRegExp("%f"), "\"" + job->location + "\"");
  command.replace(QRegExp("%o"), "\"" + desiredFile + "\"");
  
  updateProgress(job->id, 1);
  
  job->errorString = command;
  //qDebug(command.latin1());
  KShellProcess *proc = new KShellProcess();
  *proc << command.latin1();
  connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int )),
                       this, SLOT(receivedThreadOutput(KProcess *, char *, int )));
  connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int )),
                       this, SLOT(receivedThreadOutput(KProcess *, char *, int )));
  connect(proc, SIGNAL(processExited(KProcess *)), this, SLOT(jobDone(KProcess *)));
  jobs.insert(proc, job);
  threads.append(proc);

  proc->start(KShellProcess::NotifyOnExit,  KShellProcess::AllOutput);
}

/**
 * We have recieved some output from a thread. See if it contains a foo%.
 * @param proc the process that has new output. 
 * @param buffer the output from the process
 * @param buflen the length of the buffer.
 */
void EncoderConfigImp::receivedThreadOutput(KProcess *process, char *buffer, int length){
  // Make sure we have a job to send an update too.
  Job *job = jobs[(KShellProcess*)process];
  if(!job){
    qDebug(QString("EncoderConfigImp::receivedThreadOutput Job doesn't exists. Line:%1").arg(__LINE__).latin1());
    return;  
  }

  // Make sure the output string has a % symble in it.
  QString output = buffer;
  output = output.mid(0,length);
  int percentLocation = output.find('%');
  if(percentLocation==-1){
    qDebug("No Percent symbol found in output, not updating");
    return;  
  }
  //qDebug(QString("Pre cropped: %1").arg(output).latin1()); 
  output = output.mid(percentLocation-encodersPercentStringLength,2);
  //qDebug(QString("Post cropped: %1").arg(output).latin1()); 
  bool conversionSuccessfull = false;
  int percent = output.toInt(&conversionSuccessfull);
  //qDebug(QString("number: %1").arg(percent).latin1()); 
  if(percent > 0 && percent < 100 && conversionSuccessfull){
    emit(updateProgress(job->id, percent));
  }
  // If it was just some random output that couldn't be converted then don't report the error.
  else if(conversionSuccessfull){
    qDebug(QString("The Percent done (%1) is not > 0 && < 100").arg(percent).latin1());
  }
  //else{
  //  qDebug(QString("The Percent done (%1) is not > 0 && < 100, conversion ! sucesfull").arg(output).latin1());
  //}
} 

/**
 * When the thread is done encoding the file this function is called.
 * @param job the job that just finished.
 */
void EncoderConfigImp::jobDone(KProcess *process){
  if(!process)
    return;
  if(process->normalExit()){
    int retrunValue = process->exitStatus();
    if(retrunValue!=0)
      qDebug(QString("Process exited with non 0 status: %1").arg(retrunValue).latin1());
  }
  
  Job *job = jobs[(KShellProcess*)process];
  threads.remove((KShellProcess*)process);
  jobs.remove((KShellProcess*)process);

  if( QFile::exists(job->newLocation)){
    if(!saveWav->isChecked())
      QFile::remove(job->location);
    emit(updateProgress(job->id, 100));
    if(createPlaylistCheckBox->isChecked())
      appendToPlaylist(job);
  }
  else{
    KMessageBox::sorry(this, i18n("The encoded file was not created.\nPlease check your encoder options.\nThe wav file has been removed.  Command was: %1").arg(job->errorString), i18n("Encoding Failed"));
    QFile::remove(job->location);
    emit(updateProgress(job->id, -1));
  }

  delete job;
  delete process;
}

/**
 * Append the job to the playlist as specified in the options.
 * @param job too append to the playlist.
 */
void EncoderConfigImp::appendToPlaylist(Job* job){
  QString desiredFile = playlistFileFormat->text();
  replaceSpecialChars(desiredFile, job, false);
  if(desiredFile[0] == '~'){
    desiredFile.replace(0,1, QDir::homeDirPath());
  }
  int lastSlash = desiredFile.findRev('/',-1);
  if( lastSlash == -1 || !(KStandardDirs::makeDir( desiredFile.mid(0,lastSlash)))){
    KMessageBox::sorry(this, i18n("The desired encoded file could not be created.\nPlease check your file path option.\nThe wav file has been removed."), i18n("Encoding Failed"));
    QFile::remove(job->location);
    return;
  }

  QFile f(desiredFile);
  if ( !f.open(IO_WriteOnly|IO_Append) ){
    KMessageBox::sorry(this, i18n("The desired playlist file could not be opened for writing to.\nPlease check your file path option."), i18n("Playlist Addition Failed"));
    return;
  }

  QTextStream t( &f );        // use a text stream

  if(useRelitivePath->isChecked()){
    KURL audioFile(job->newLocation);
    t << "./" << audioFile.fileName() << endl;
  }
  else{
    t << job->newLocation << endl;
  }
  f.close();

}

/**
 * Load up the wizard with the playlist string.  Save it if OK is hit. 
 */
void EncoderConfigImp::playlistWizard(){
  fileWizard wizard(this, "Playlist File FormatWizard", true);
  wizard.playlistFormat->setText(playlistFileFormat->text());
  
  // Show dialog and save results if ok is pressed.
  bool okClicked = wizard.exec();
  if(okClicked){
    playlistFileFormat->setText(wizard.playlistFormat->text());
  }
}

/**
 * Load up the wizard with the encoder playlist string.  Save it if OK is hit. 
 */
void EncoderConfigImp::encoderWizard(){
  fileWizard wizard(this, "Encoder File Format Wizard", true);
  wizard.playlistFormat->setText(fileFormat->text());
  
  // Show dialog and save results if ok is pressed.
  bool okClicked = wizard.exec();
  if(okClicked){
    fileFormat->setText(wizard.playlistFormat->text());
  }
}


#include "encoderconfigimp.moc"

// encoderconfigimp.cpp

