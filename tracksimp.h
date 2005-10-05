/**
 * Copyright (C) 2003-2005 Benjamin C Meyer (ben at meyerhome dot net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef TRACKSIMP_H
#define TRACKSIMP_H

#include "tracks.h"
#include <klocale.h>

// CDDB support via libkcddb
#include <libkcddb/client.h>
//Added by qt3to4:
#include <QKeyEvent>

using namespace KCDDB;
class Job;
class KProcess;
class KCompactDisc;

/**
 * This class handles the display of the tracks. It also starts off the job que.
 */
class TracksImp : public Tracks {

Q_OBJECT

signals:
	void ripTrack(Job *job);
	void hasCD(bool);
	void hasTracks(bool);
 
public:
	TracksImp( QWidget* parent = 0, const char* name = 0);
	~TracksImp();

	bool hasCD();

public slots:
	void loadSettings();

	// Toolbar Buttons
	void startSession();
	void startSession( int encoder );
	void editInformation();
	void performCDDB();
	void ejectDevice(const QString &deviceToEject);
	void eject();
	void selectAllTracks();
	void deselectAllTracks();

private slots:
	void newDisc(unsigned discId);
 
	void selectTrack(Q3ListViewItem *);
	void keyPressEvent(QKeyEvent *event);
 
	void changeDevice(const QString &file);
	void lookupCDDBDone(CDDB::Result result);

private:
	void lookupDevice();
	void lookupCDDB();
	void newAlbum();
	void ripWholeAlbum();
	QString formatTime(unsigned ms);

	KCDDB::Client* cddb;

	KCompactDisc* cd;

	// Current album
	KCDDB::CDInfo cddbInfo;
};

#endif // TRACKSIMP_H
