///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

#include <MUtils/Global.h>
#include <MUtils/UpdateChecker.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>

using namespace MUtils;

///////////////////////////////////////////////////////////////////////////////
// CONSTANTS
///////////////////////////////////////////////////////////////////////////////

static const char *header_id = "!Update";
static const char *section_id = "LameXP";

static const char *mirror_url_postfix[] = 
{
	"update.ver",
	"update_beta.ver",
	NULL
};

static const char *update_mirrors_prim[] =
{
	"http://muldersoft.com/",
	"http://mulder.bplaced.net/",
	"http://mulder.cwsurf.de/",
	"http://mulder.6te.net/",
	"http://mulder.webuda.com/",
	"http://mulder.byethost13.com/",
	"http://muldersoft.kilu.de/",
	"http://mulder.pe.hu/",
	"http://muldersoft.zxq.net/",
	"http://lamexp.sourceforge.net/",
	"http://lordmulder.github.io/LameXP/",
	"http://lord_mulder.bitbucket.org/",
	"http://www.tricksoft.de/",
	NULL
};

static const char *update_mirrors_back[] =
{
	"http://mplayer.savedonthe.net/",
	NULL
};

static const char *known_hosts[] =		//Taken form: http://www.alexa.com/topsites !!!
{
	"http://www.163.com/",
	"http://www.7-zip.org/",
	"http://www.ac3filter.net/",
	"http://www.amazon.com/",
	"http://antergos.com/",
	"http://www.aol.com/",
	"http://www.apache.org/",
	"http://www.apple.com/",
	"http://www.adobe.com/",
	"http://www.avidemux.org/",
	"http://www.babylon.com/",
	"http://www.baidu.com/",
	"http://bandcamp.com/",
	"http://www.bbc.co.uk/",
	"http://www.berlios.de/",
	"http://www.bing.com/",
	"http://www.cnet.com/",
	"http://cnzz.com/",
	"http://www.codeplex.com/",
	"http://qt.digia.com/",
	"http://www.ebay.com/",
	"http://www.equation.com/",
	"http://fc2.com/",
	"http://fedoraproject.org/",
	"http://www.ffmpeg.org/",
	"http://blog.flickr.net/en",
	"http://free-codecs.com/",
	"http://blog.gitorious.org/",
	"http://git-scm.com/",
	"http://www.gmx.net/",
	"http://www.gnome.org/",
	"http://www.gnu.org/",
	"http://go.com/",
	"http://code.google.com/",
	"http://www.heise.de/",
	"http://www.huffingtonpost.co.uk/",
	"http://www.iana.org/",
	"http://www.imdb.com/",
	"http://www.imgburn.com/",
	"http://imgur.com/",
	"http://en.jd.com/",
	"http://kannmanumdieuhrzeitschonnbierchentrinken.de/",
	"http://mirrors.kernel.org/",
	"http://lame.sourceforge.net/",
	"http://www.libav.org/",
	"http://www.linkedin.com/about-us",
	"http://www.linuxmint.com/",
	"http://www.livedoor.com/",
	"http://www.livejournal.com/",
	"http://go.mail.ru/",
	"http://www.mediafire.com/",
	"http://ftp.mozilla.org/",
	"http://mplayerhq.hu/",
	"http://www.msn.com/?st=1",
	"http://oss.netfarm.it/",
	"http://www.nytimes.com/",
	"http://www.opera.com/",
	"http://pastie.org/",
	"http://www.portablefreeware.com/",
	"http://qt-project.org/",
	"http://www.quakelive.com/",
	"http://www.seamonkey-project.org/",
	"http://www.sina.com.cn/",
	"http://www.sohu.com/",
	"http://www.soso.com/",
	"http://sourceforge.net/",
	"http://www.spiegel.de/",
	"http://tdm-gcc.tdragon.net/",
	"http://www.tdrsmusic.com/",
	"http://www.ubuntu.com/",
	"http://status.twitter.com/",
	"http://www.uol.com.br/",
	"http://www.videohelp.com/",
	"http://www.videolan.org/",
	"http://virtualdub.org/",
	"http://blog.virustotal.com/",
	"http://weibo.com/login.php",
	"http://www.wikipedia.org/",
	"http://www.winamp.com/",
	"http://wordpress.com/about/",
	"http://xiph.org/",
	"http://us.mail.yahoo.com/",
	"http://www.yandex.ru/",
	"http://www.youtube.com/yt/about/",
	"http://www.zedo.com/",
	"http://ffmpeg.zeranoe.com/",
	NULL
};

static const int MIN_CONNSCORE = 8;
static const int VERSION_INFO_EXPIRES_MONTHS = 6;
static char *USER_AGENT_STR = "Mozilla/5.0 (X11; Linux i686; rv:7.0.1) Gecko/20111106 IceCat/7.0.1";

//Helper function
static int getMaxProgress(void)
{
	int counter = MIN_CONNSCORE + 2;
	for(int i = 0; update_mirrors_prim[i]; i++) counter++;
	for(int i = 0; update_mirrors_back[i]; i++) counter++;
	return counter;
}

////////////////////////////////////////////////////////////
// Update Info Class
////////////////////////////////////////////////////////////

UpdateCheckerInfo::UpdateCheckerInfo(void)
{
	resetInfo();
}
	
void UpdateCheckerInfo::resetInfo(void)
{
	m_buildNo = 0;
	m_buildDate.setDate(1900, 1, 1);
	m_downloadSite.clear();
	m_downloadAddress.clear();
	m_downloadFilename.clear();
	m_downloadFilecode.clear();
}

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

UpdateChecker::UpdateChecker(const QString &binWGet, const QString &binGnuPG, const QString &binKeys, const quint32 &installedBuildNo, const bool betaUpdates, const bool testMode)
:
	m_updateInfo(new UpdateCheckerInfo()),
	m_binaryWGet(binWGet),
	m_binaryGnuPG(binGnuPG),
	m_binaryKeys(binKeys),
	m_installedBuildNo(installedBuildNo),
	m_betaUpdates(betaUpdates),
	m_testMode(testMode),
	m_maxProgress(getMaxProgress())
{
	m_success = false;
	m_status = UpdateStatus_NotStartedYet;
	m_progress = 0;

	if(m_binaryWGet.isEmpty() || m_binaryGnuPG.isEmpty() || m_binaryKeys.isEmpty())
	{
		MUTILS_THROW("Tools not initialized correctly!");
	}
}

UpdateChecker::~UpdateChecker(void)
{
	delete m_updateInfo;
}

////////////////////////////////////////////////////////////
// Protected functions
////////////////////////////////////////////////////////////

void UpdateChecker::run(void)
{
	qDebug("Update checker thread started!");
	MUTILS_EXCEPTION_HANDLER(m_testMode ? testKnownHosts() : checkForUpdates());
	qDebug("Update checker thread completed.");
}

void UpdateChecker::checkForUpdates(void)
{
	// ----- Initialization ----- //

	m_success = false;
	m_updateInfo->resetInfo();
	seed_rand();
	setProgress(0);

	// ----- Test Internet Connection ----- //

	int connectionScore = 0;
	int maxConnectTries = (3 * MIN_CONNSCORE) / 2;
	
	log("Checking internet connection...");
	setStatus(UpdateStatus_CheckingConnection);

	const int networkStatus = lamexp_network_status();
	if(networkStatus == lamexp_network_non)
	{
		log("", "Operating system reports that the computer is currently offline !!!");
		setProgress(m_maxProgress);
		setStatus(UpdateStatus_ErrorNoConnection);
		return;
	}
	
	setProgress(1);

	// ----- Test Known Hosts Connectivity ----- //

	QStringList hostList;
	for(int i = 0; known_hosts[i]; i++)
	{
		hostList << QString::fromLatin1(known_hosts[i]);
	}

	while(!(hostList.isEmpty() || (connectionScore >= MIN_CONNSCORE) || (maxConnectTries < 1)))
	{
		switch(tryContactHost(hostList.takeAt(next_rand32() % hostList.count())))
		{
			case 01: connectionScore += 1; break;
			case 02: connectionScore += 2; break;
			default: maxConnectTries -= 1; break;
		}
		setProgress(qBound(1, connectionScore + 1, MIN_CONNSCORE + 1));
		msleep(64);
	}

	if(connectionScore < MIN_CONNSCORE)
	{
		log("", "Connectivity test has failed: Internet connection appears to be broken!");
		setProgress(m_maxProgress);
		setStatus(UpdateStatus_ErrorConnectionTestFailed);
		return;
	}

	// ----- Build Mirror List ----- //

	log("", "----", "", "Checking for updates online...");
	setStatus(UpdateStatus_FetchingUpdates);

	QStringList mirrorList;
	for(int index = 0; update_mirrors_prim[index]; index++)
	{
		mirrorList << QString::fromLatin1(update_mirrors_prim[index]);
	}

	if(const int len = mirrorList.count())
	{
		const int rounds = len * 1097;
		for(int i = 0; i < rounds; i++)
		{
			mirrorList.swap(i % len, next_rand32() % len);
		}
	}

	for(int index = 0; update_mirrors_back[index]; index++)
	{
		mirrorList << QString::fromLatin1(update_mirrors_back[index]);
	}
	
	// ----- Fetch Update Info From Server ----- //

	while(!mirrorList.isEmpty())
	{
		QString currentMirror = mirrorList.takeFirst();
		setProgress(m_progress + 1);
		if(!m_success)
		{
			if(tryUpdateMirror(m_updateInfo, currentMirror))
			{
				m_success = true;
			}
		}
		else
		{
			msleep(64);
		}
	}
	
	setProgress(m_maxProgress);

	if(m_success)
	{
		if(m_updateInfo->m_buildNo > m_installedBuildNo)
		{
			setStatus(UpdateStatus_CompletedUpdateAvailable);
		}
		else if(m_updateInfo->m_buildNo == m_installedBuildNo)
		{
			setStatus(UpdateStatus_CompletedNoUpdates);
		}
		else
		{
			setStatus(UpdateStatus_CompletedNewVersionOlder);
		}
	}
	else
	{
		setStatus(UpdateStatus_ErrorFetchUpdateInfo);
	}
}

void UpdateChecker::testKnownHosts(void)
{
	QStringList hostList;
	for(int i = 0; known_hosts[i]; i++)
	{
		hostList << QString::fromLatin1(known_hosts[i]);
	}

	qDebug("\n[Known Hosts]");
	log("Testing all known hosts...", "", "---");

	int hostCount = hostList.count();
	while(!hostList.isEmpty())
	{
		QString currentHost = hostList.takeFirst();
		qDebug("Testing: %s", currentHost.toLatin1().constData());
		log("", "Testing:", currentHost, "");
		QString outFile = QString("%1/%2.htm").arg(temp_folder(), rand_str());
		bool httpOk = false;
		if(!getFile(currentHost, outFile, 0, &httpOk))
		{
			if(httpOk)
			{
				qWarning("\nConnectivity test was SLOW on the following site:\n%s\n", currentHost.toLatin1().constData());
			}
			else
			{
				qWarning("\nConnectivity test FAILED on the following site:\n%s\n", currentHost.toLatin1().constData());
			}
		}
		log("", "---");
		QFile::remove(outFile);
	}
}

////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////

void UpdateChecker::setStatus(const int status)
{
	if(m_status != status)
	{
		m_status = status;
		emit statusChanged(status);
	}
}

void UpdateChecker::setProgress(const int progress)
{
	if(m_progress != progress)
	{
		m_progress = progress;
		emit progressChanged(progress);
	}
}

void UpdateChecker::log(const QString &str1, const QString &str2, const QString &str3, const QString &str4)
{
	if(!str1.isNull()) emit messageLogged(str1);
	if(!str2.isNull()) emit messageLogged(str2);
	if(!str3.isNull()) emit messageLogged(str3);
	if(!str4.isNull()) emit messageLogged(str4);
}

int UpdateChecker::tryContactHost(const QString &url)
{
		int result = -1; bool httpOkay = false;
		const QString outFile = QString("%1/%2.htm").arg(temp_folder(), rand_str());
		log("", "Testing host:", url);

		if(getFile(url, outFile, 0, &httpOkay))
		{
			log("Connection to host was established successfully.");
			result = 2;
		}
		else
		{
			if(httpOkay)
			{
				log("Connection to host timed out after HTTP OK was received.");
				result = 1;
			}
			else
			{
				log("Connection failed: The host could not be reached!");
				result = 0;
			}
		}

		QFile::remove(outFile);
		return result;
}

bool UpdateChecker::tryUpdateMirror(UpdateCheckerInfo *updateInfo, const QString &url)
{
	bool success = false;
	log("", "Trying mirror:", url);

	const QString randPart = rand_str();
	const QString outFileVers = QString("%1/%2.ver").arg(temp_folder(), randPart);
	const QString outFileSign = QString("%1/%2.sig").arg(temp_folder(), randPart);

	if(getUpdateInfo(url, outFileVers, outFileSign))
	{
		log("", "Download okay, checking signature:");
		if(checkSignature(outFileVers, outFileSign))
		{
			log("", "Signature okay, parsing info:");
			success = parseVersionInfo(outFileVers, updateInfo);
		}
		else
		{
			log("", "Bad signature, take care!");
		}
	}
	else
	{
		log("", "Download has failed!");
	}

	QFile::remove(outFileVers);
	QFile::remove(outFileSign);
	
	return success;
}

bool UpdateChecker::getUpdateInfo(const QString &url, const QString &outFileVers, const QString &outFileSign)
{
	log("", "Downloading update info:");
	if(!getFile(QString("%1%2"    ).arg(url, mirror_url_postfix[m_betaUpdates ? 1 : 0]), outFileVers))
	{
		return false;
	}

	log("", "Downloading signature:");
	if(!getFile(QString("%1%2.sig").arg(url, mirror_url_postfix[m_betaUpdates ? 1 : 0]), outFileSign))
	{
		return false;
	}

	return true;
}

bool UpdateChecker::getFile(const QString &url, const QString &outFile, unsigned int maxRedir, bool *httpOk)
{
	QFileInfo output(outFile);
	output.setCaching(false);
	if(httpOk) *httpOk = false;

	if(output.exists())
	{
		QFile::remove(output.canonicalFilePath());
		if(output.exists())
		{
			return false;
		}
	}

	QProcess process;
	init_process(process, output.absolutePath());

	QStringList args;
	args << "-T" << "15" << "--no-cache" << "--no-dns-cache" << QString().sprintf("--max-redirect=%u", maxRedir);
	args << QString("--referer=%1://%2/").arg(QUrl(url).scheme(), QUrl(url).host()) << "-U" << USER_AGENT_STR;
	args << "-O" << output.fileName() << url;

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));

	QTimer timer;
	timer.setSingleShot(true);
	timer.setInterval(25000);
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	const QRegExp httpResponseOK("200 OK$");
	
	process.start(m_binaryWGet, args);
	
	if(!process.waitForStarted())
	{
		return false;
	}

	timer.start();

	while(process.state() != QProcess::NotRunning)
	{
		loop.exec();
		const bool bTimeOut = (!timer.isActive());
		while(process.canReadLine())
		{
			QString line = QString::fromLatin1(process.readLine()).simplified();
			if(line.contains(httpResponseOK))
			{
				line.append(" [OK]");
				if(httpOk) *httpOk = true;
			}
			log(line);
		}
		if(bTimeOut)
		{
			qWarning("WGet process timed out <-- killing!");
			process.kill();
			process.waitForFinished();
			log("!!! TIMEOUT !!!");
			return false;
		}
	}
	
	timer.stop();
	timer.disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	log(QString().sprintf("Exited with code %d", process.exitCode()));
	return (process.exitCode() == 0) && output.exists() && output.isFile();
}

bool UpdateChecker::checkSignature(const QString &file, const QString &signature)
{
	if(QFileInfo(file).absolutePath().compare(QFileInfo(signature).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		qWarning("CheckSignature: File and signature should be in same folder!");
		return false;
	}
	if(QFileInfo(file).absolutePath().compare(QFileInfo(m_binaryKeys).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		qWarning("CheckSignature: File and keyring should be in same folder!");
		return false;
	}

	QProcess process;
	init_process(process, QFileInfo(file).absolutePath());

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));
	
	process.start(m_binaryGnuPG, QStringList() << "--homedir" << "." << "--keyring" << QFileInfo(m_binaryKeys).fileName() << QFileInfo(signature).fileName() << QFileInfo(file).fileName());

	if(!process.waitForStarted())
	{
		return false;
	}

	while(process.state() == QProcess::Running)
	{
		loop.exec();
		while(process.canReadLine())
		{
			log(QString::fromLatin1(process.readLine()).simplified());
		}
	}
	
	log(QString().sprintf("Exited with code %d", process.exitCode()));
	return (process.exitCode() == 0);
}

bool UpdateChecker::parseVersionInfo(const QString &file, UpdateCheckerInfo *updateInfo)
{
	QRegExp value("^(\\w+)=(.+)$");
	QRegExp section("^\\[(.+)\\]$");

	QDate updateInfoDate;
	updateInfo->resetInfo();

	QFile data(file);
	if(!data.open(QIODevice::ReadOnly))
	{
		qWarning("Cannot open update info file for reading!");
		return false;
	}
	
	bool inHeader = false;
	bool inSection = false;
	
	while(!data.atEnd())
	{
		QString line = QString::fromLatin1(data.readLine()).trimmed();
		if(section.indexIn(line) >= 0)
		{
			log(QString("Sec: [%1]").arg(section.cap(1)));
			inSection = (section.cap(1).compare(section_id, Qt::CaseInsensitive) == 0);
			inHeader = (section.cap(1).compare(header_id, Qt::CaseInsensitive) == 0);
			continue;
		}
		if(inSection && (value.indexIn(line) >= 0))
		{
			log(QString("Val: '%1' ==> '%2").arg(value.cap(1), value.cap(2)));
			if(value.cap(1).compare("BuildNo", Qt::CaseInsensitive) == 0)
			{
				bool ok = false;
				unsigned int temp = value.cap(2).toUInt(&ok);
				if(ok) updateInfo->m_buildNo = temp;
			}
			else if(value.cap(1).compare("BuildDate", Qt::CaseInsensitive) == 0)
			{
				QDate temp = QDate::fromString(value.cap(2).trimmed(), Qt::ISODate);
				if(temp.isValid()) updateInfo->m_buildDate = temp;
			}
			else if(value.cap(1).compare("DownloadSite", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadSite = value.cap(2).trimmed();
			}
			else if(value.cap(1).compare("DownloadAddress", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadAddress = value.cap(2).trimmed();
			}
			else if(value.cap(1).compare("DownloadFilename", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadFilename = value.cap(2).trimmed();
			}
			else if(value.cap(1).compare("DownloadFilecode", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadFilecode = value.cap(2).trimmed();
			}
		}
		if(inHeader && (value.indexIn(line) >= 0))
		{
			log(QString("Val: '%1' ==> '%2").arg(value.cap(1), value.cap(2)));
			if(value.cap(1).compare("TimestampCreated", Qt::CaseInsensitive) == 0)
			{
				QDate temp = QDate::fromString(value.cap(2).trimmed(), Qt::ISODate);
				if(temp.isValid()) updateInfoDate = temp;
			}
		}
	}

	if(!updateInfoDate.isValid())
	{
		updateInfo->resetInfo();
		log("WARNING: Version info timestamp is missing!");
		return false;
	}
	
	const QDate currentDate = OS::current_date();
	if(updateInfoDate.addMonths(VERSION_INFO_EXPIRES_MONTHS) < currentDate)
	{
		updateInfo->resetInfo();
		log(QString::fromLatin1("WARNING: This version info has expired at %1!").arg(updateInfoDate.addMonths(VERSION_INFO_EXPIRES_MONTHS).toString(Qt::ISODate)));
		return false;
	}
	else if(currentDate < updateInfoDate)
	{
		log("Version info is from the future, take care!");
		qWarning("Version info is from the future, take care!");
	}

	bool complete = true;

	if(!(updateInfo->m_buildNo > 0)) complete = false;
	if(!(updateInfo->m_buildDate.year() >= 2010)) complete = false;
	if(updateInfo->m_downloadSite.isEmpty()) complete = false;
	if(updateInfo->m_downloadAddress.isEmpty()) complete = false;
	if(updateInfo->m_downloadFilename.isEmpty()) complete = false;
	if(updateInfo->m_downloadFilecode.isEmpty()) complete = false;
	
	if(!complete)
	{
		log("WARNING: Version info is incomplete!");
	}

	return complete;
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
