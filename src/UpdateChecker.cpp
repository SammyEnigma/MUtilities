///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <QElapsedTimer>
#include <QSet>

#include "Mirrors.h"

///////////////////////////////////////////////////////////////////////////////
// CONSTANTS
///////////////////////////////////////////////////////////////////////////////

static const char *header_id = "!Update";

static const char *mirror_url_postfix[] = 
{
	"update.ver",
	"update_beta.ver",
	NULL
};

static const int MIN_CONNSCORE = 5;
static const int QUICK_MIRRORS = 3;
static const int MAX_CONN_TIMEOUT = 16000;
static const int DOWNLOAD_TIMEOUT = 30000;

static const int VERSION_INFO_EXPIRES_MONTHS = 6;
static char *USER_AGENT_STR = "Mozilla/5.0 (X11; Linux i686; rv:10.0) Gecko/20100101 Firefox/10.0"; /*use something innocuous*/

#define CHECK_CANCELLED() do \
{ \
	if(MUTILS_BOOLIFY(m_cancelled)) \
	{ \
		m_success.fetchAndStoreOrdered(0); \
		log("", "Update check has been cancelled by user!"); \
		setProgress(m_maxProgress); \
		setStatus(UpdateStatus_CancelledByUser); \
		return; \
	} \
} \
while(0)

////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////

static int getMaxProgress(void)
{
	int counter = 0;
	while (update_mirrors[counter])
	{
		counter++;
	}
	counter += MIN_CONNSCORE + QUICK_MIRRORS + 2;
	return counter; ;
}

static QStringList buildRandomList(const char *const values[])
{
	QStringList list;
	for (int index = 0; values[index]; index++)
	{
		const int pos = MUtils::next_rand_u32() % (index + 1);
		list.insert(pos, QString::fromLatin1(values[index]));
	}
	return list;
}

////////////////////////////////////////////////////////////
// Update Info Class
////////////////////////////////////////////////////////////

MUtils::UpdateCheckerInfo::UpdateCheckerInfo(void)
{
	resetInfo();
}
	
void MUtils::UpdateCheckerInfo::resetInfo(void)
{
	m_buildNo = 0;
	m_buildDate.setDate(1900, 1, 1);
	m_downloadSite.clear();
	m_downloadAddress.clear();
	m_downloadFilename.clear();
	m_downloadFilecode.clear();
	m_downloadChecksum.clear();
}

bool MUtils::UpdateCheckerInfo::isComplete(void)
{
	if(this->m_buildNo < 1)                return false;
	if(this->m_buildDate.year() < 2010)    return false;
	if(this->m_downloadSite.isEmpty())     return false;
	if(this->m_downloadAddress.isEmpty())  return false;
	if(this->m_downloadFilename.isEmpty()) return false;
	if(this->m_downloadFilecode.isEmpty()) return false;
	if(this->m_downloadChecksum.isEmpty()) return false;

	return true;
}

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

MUtils::UpdateChecker::UpdateChecker(const QString &binWGet, const QString &binMCat, const QString &binGnuPG, const QString &binKeys, const QString &applicationId, const quint32 &installedBuildNo, const bool betaUpdates, const bool testMode)
:
	m_updateInfo(new UpdateCheckerInfo()),
	m_binaryWGet(binWGet),
	m_binaryMCat(binMCat),
	m_binaryGnuPG(binGnuPG),
	m_binaryKeys(binKeys),
	m_applicationId(applicationId),
	m_installedBuildNo(installedBuildNo),
	m_betaUpdates(betaUpdates),
	m_testMode(testMode),
	m_maxProgress(getMaxProgress())
{
	m_status = UpdateStatus_NotStartedYet;
	m_progress = 0;

	if(m_binaryWGet.isEmpty() || m_binaryGnuPG.isEmpty() || m_binaryKeys.isEmpty())
	{
		MUTILS_THROW("Tools not initialized correctly!");
	}
}

MUtils::UpdateChecker::~UpdateChecker(void)
{
}

////////////////////////////////////////////////////////////
// Public slots
////////////////////////////////////////////////////////////

void MUtils::UpdateChecker::start(Priority priority)
{
	m_success.fetchAndStoreOrdered(0);
	m_cancelled.fetchAndStoreOrdered(0);
	QThread::start(priority);
}

////////////////////////////////////////////////////////////
// Protected functions
////////////////////////////////////////////////////////////

void MUtils::UpdateChecker::run(void)
{
	qDebug("Update checker thread started!");
	MUTILS_EXCEPTION_HANDLER(m_testMode ? testMirrorsList() : checkForUpdates());
	qDebug("Update checker thread completed.");
}

void MUtils::UpdateChecker::checkForUpdates(void)
{
	// ----- Initialization ----- //

	m_updateInfo->resetInfo();
	setProgress(0);

	// ----- Test Internet Connection ----- //

	log("Checking internet connection...", "");
	setStatus(UpdateStatus_CheckingConnection);

	const int networkStatus = OS::network_status();
	if(networkStatus == OS::NETWORK_TYPE_NON)
	{
		log("Operating system reports that the computer is currently offline !!!");
		setProgress(m_maxProgress);
		setStatus(UpdateStatus_ErrorNoConnection);
		return;
	}
	
	setProgress(1);

	// ----- Test Known Hosts Connectivity ----- //

	int connectionScore = 0;
	QStringList mirrorList = buildRandomList(known_hosts);

	for(int connectionTimout = 500; connectionTimout <= MAX_CONN_TIMEOUT; connectionTimout *= 2)
	{
		QElapsedTimer elapsedTimer;
		elapsedTimer.start();
		const int globalTimout = 2 * MIN_CONNSCORE * connectionTimout;
		while (!elapsedTimer.hasExpired(globalTimout))
		{
			const QString hostName = mirrorList.takeFirst();
			if (tryContactHost(hostName, connectionTimout))
			{
				connectionScore += 1;
				setProgress(qBound(1, connectionScore + 1, MIN_CONNSCORE + 1));
				elapsedTimer.restart();
				if (connectionScore >= MIN_CONNSCORE)
				{
					goto endLoop; /*success*/
				}
			}
			else
			{
				mirrorList.append(hostName); /*re-schedule*/
			}
			CHECK_CANCELLED();
			msleep(1);
		}
	}

endLoop:
	if(connectionScore < MIN_CONNSCORE)
	{
		log("", "Connectivity test has failed: Internet connection appears to be broken!");
		setProgress(m_maxProgress);
		setStatus(UpdateStatus_ErrorConnectionTestFailed);
		return;
	}

	// ----- Fetch Update Info From Server ----- //

	log("----", "", "Checking for updates online...");
	setStatus(UpdateStatus_FetchingUpdates);

	int mirrorCount = 0;
	mirrorList = buildRandomList(update_mirrors);

	while(!mirrorList.isEmpty())
	{
		setProgress(m_progress + 1);
		const QString currentMirror = mirrorList.takeFirst();
		const bool isQuick = (mirrorCount++ < QUICK_MIRRORS);
		if(tryUpdateMirror(m_updateInfo.data(), currentMirror, isQuick))
		{
			m_success.ref(); /*success*/
			break;
		}
		if (isQuick)
		{
			mirrorList.append(currentMirror); /*re-schedule*/
		}
		CHECK_CANCELLED();
		msleep(1);
	}
	
	while (m_progress < m_maxProgress)
	{
		msleep(16);
		setProgress(m_progress + 1);
		CHECK_CANCELLED();
	}

	// ----- Generate final result ----- //

	if(MUTILS_BOOLIFY(m_success))
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

void MUtils::UpdateChecker::testMirrorsList(void)
{
	// ----- Test update mirrors ----- //

	QStringList mirrorList;
	for(int i = 0; update_mirrors[i]; i++)
	{
		mirrorList << QString::fromLatin1(update_mirrors[i]);
	}

	qDebug("\n[Mirror Sites]");
	log("Testing all known mirror sites...", "", "---");

	UpdateCheckerInfo updateInfo;
	while (!mirrorList.isEmpty())
	{
		const QString currentMirror = mirrorList.takeFirst();
		bool success = false;
		qDebug("Testing: %s", MUTILS_L1STR(currentMirror));
		log("", "Testing:", currentMirror, "");
		for (quint8 attempt = 0; attempt < 3; ++attempt)
		{
			updateInfo.resetInfo();
			if (tryUpdateMirror(&updateInfo, currentMirror, (!attempt)))
			{
				success = true;
				break;
			}
		}
		if (!success)
		{
			qWarning("\nUpdate mirror seems to be unavailable:\n%s\n", MUTILS_L1STR(currentMirror));
		}
		log("", "---");
	}

	// ----- Test known hosts ----- //

	QStringList knownHostList;
	for (int i = 0; known_hosts[i]; i++)
	{
		knownHostList << QString::fromLatin1(known_hosts[i]);
	}

	qDebug("\n[Known Hosts]");
	log("Testing all known hosts...", "", "---");

	QSet<quint32> ipAddrSet;
	quint32 ipAddr;
	while(!knownHostList.isEmpty())
	{
		const QString currentHost = knownHostList.takeFirst();
		qDebug("Testing: %s", MUTILS_L1STR(currentHost));
		log(QLatin1String(""), "Testing:", currentHost, "");
		if (tryContactHost(currentHost, DOWNLOAD_TIMEOUT, &ipAddr))
		{
			if (ipAddrSet.contains(ipAddr))
			{
				qWarning("Duplicate IP-address 0x%08X was encountered!", ipAddr);
			}
			else
			{
				ipAddrSet << ipAddr; /*not encountered yet*/
			}
		}
		else
		{
			qWarning("\nConnectivity test FAILED on the following host:\n%s\n", MUTILS_L1STR(currentHost));
		}
		log("", "---");
	}
}

////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////

void MUtils::UpdateChecker::setStatus(const int status)
{
	if(m_status != status)
	{
		m_status = status;
		emit statusChanged(status);
	}
}

void MUtils::UpdateChecker::setProgress(const int progress)
{
	if(m_progress != progress)
	{
		m_progress = progress;
		emit progressChanged(progress);
	}
}

void MUtils::UpdateChecker::log(const QString &str1, const QString &str2, const QString &str3, const QString &str4)
{
	if(!str1.isNull()) emit messageLogged(str1);
	if(!str2.isNull()) emit messageLogged(str2);
	if(!str3.isNull()) emit messageLogged(str3);
	if(!str4.isNull()) emit messageLogged(str4);
}

bool MUtils::UpdateChecker::tryUpdateMirror(UpdateCheckerInfo *updateInfo, const QString &url, const bool &quick)
{
	bool success = false;
	log("", "Trying mirror:", url, "");

	if (quick)
	{
		if (!tryContactHost(QUrl(url).host(), (MAX_CONN_TIMEOUT / 10)))
		{
			log("", "Mirror is too slow, skipping!");
			return false;
		}
	}

	const QString randPart = next_rand_str();
	const QString outFileVers = QString("%1/%2.ver").arg(temp_folder(), randPart);
	const QString outFileSign = QString("%1/%2.sig").arg(temp_folder(), randPart);

	if (getUpdateInfo(url, outFileVers, outFileSign))
	{
		log("", "Download okay, checking signature:");
		if (checkSignature(outFileVers, outFileSign))
		{
			log("", "Signature okay, parsing info:", "");
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

bool MUtils::UpdateChecker::getUpdateInfo(const QString &url, const QString &outFileVers, const QString &outFileSign)
{
	log("Downloading update info:", "");
	if(getFile(QString("%1%2").arg(url, mirror_url_postfix[m_betaUpdates ? 1 : 0]), outFileVers))
	{
		if (!m_cancelled)
		{
			log("", "Downloading signature:", "");
			if (getFile(QString("%1%2.sig2").arg(url, mirror_url_postfix[m_betaUpdates ? 1 : 0]), outFileSign))
			{
				return true;
			}
		}
	}
	return false;
}

bool MUtils::UpdateChecker::parseVersionInfo(const QString &file, UpdateCheckerInfo *updateInfo)
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
	
	bool inHdr = false;
	bool inSec = false;
	
	while(!data.atEnd())
	{
		QString line = QString::fromLatin1(data.readLine()).trimmed();
		if(section.indexIn(line) >= 0)
		{
			log(QString("Sec: [%1]").arg(section.cap(1)));
			inSec = (section.cap(1).compare(m_applicationId, Qt::CaseInsensitive) == 0);
			inHdr = (section.cap(1).compare(QString::fromLatin1(header_id), Qt::CaseInsensitive) == 0);
			continue;
		}
		if(inSec && (value.indexIn(line) >= 0))
		{
			log(QString("Val: '%1' ==> '%2").arg(value.cap(1), value.cap(2)));
			if(value.cap(1).compare("BuildNo", Qt::CaseInsensitive) == 0)
			{
				bool ok = false;
				const unsigned int temp = value.cap(2).toUInt(&ok);
				if(ok) updateInfo->m_buildNo = temp;
			}
			else if(value.cap(1).compare("BuildDate", Qt::CaseInsensitive) == 0)
			{
				const QDate temp = QDate::fromString(value.cap(2).trimmed(), Qt::ISODate);
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
			else if(value.cap(1).compare("DownloadChecksum", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadChecksum = value.cap(2).trimmed();
			}
		}
		if(inHdr && (value.indexIn(line) >= 0))
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
	
	if(!updateInfo->isComplete())
	{
		log("WARNING: Version info is incomplete!");
		return false;
	}

	return true;
}

//----------------------------------------------------------
// EXTERNAL TOOLS
//----------------------------------------------------------

bool MUtils::UpdateChecker::getFile(const QString &url, const QString &outFile, const unsigned int maxRedir)
{
	for (int i = 0; i < 2; i++)
	{
		if (getFile(url, (i > 0), outFile, maxRedir))
		{
			return true;
		}
		if (MUTILS_BOOLIFY(m_cancelled))
		{
			break; /*cancelled*/
		}
	}
	return false;
}

bool MUtils::UpdateChecker::getFile(const QString &url, const bool forceIp4, const QString &outFile, const unsigned int maxRedir)
{
	QFileInfo output(outFile);
	output.setCaching(false);

	if (output.exists())
	{
		QFile::remove(output.canonicalFilePath());
		if (output.exists())
		{
			return false;
		}
	}

	QProcess process;
	init_process(process, output.absolutePath());

	QStringList args;
	if (forceIp4)
	{
		args << "-4";
	}

	args << "--no-config" << "--no-cache" << "--no-dns-cache" << "--no-check-certificate" << "--no-hsts";
	args << QString().sprintf("--max-redirect=%u", maxRedir) << QString().sprintf("--timeout=%u", DOWNLOAD_TIMEOUT / 1000);
	args << QString("--referer=%1://%2/").arg(QUrl(url).scheme(), QUrl(url).host()) << "-U" << USER_AGENT_STR;
	args << "-O" << output.fileName() << url;

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));

	QTimer timer;
	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	const QRegExp httpResponseOK("200 OK$");

	process.start(m_binaryWGet, args);

	if (!process.waitForStarted())
	{
		return false;
	}

	timer.start(DOWNLOAD_TIMEOUT);

	while (process.state() != QProcess::NotRunning)
	{
		loop.exec();
		const bool bTimeOut = (!timer.isActive());
		while (process.canReadLine())
		{
			const QString line = QString::fromLatin1(process.readLine()).simplified();
			log(line);
		}
		if (bTimeOut || MUTILS_BOOLIFY(m_cancelled))
		{
			qWarning("WGet process timed out <-- killing!");
			process.kill();
			process.waitForFinished();
			log(bTimeOut ? "!!! TIMEOUT !!!": "!!! CANCELLED !!!");
			return false;
		}
	}

	timer.stop();
	timer.disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	log(QString().sprintf("Exited with code %d", process.exitCode()));
	return (process.exitCode() == 0) && output.exists() && output.isFile();
}

bool MUtils::UpdateChecker::tryContactHost(const QString &hostname, const int &timeoutMsec, quint32 *const ipAddrOut)
{
	log(QString("Connecting to host: %1").arg(hostname), "");

	QProcess process;
	init_process(process, temp_folder());

	QStringList args;
	args << "--retry" << QString::number(3) << hostname << QString::number(80);

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));

	QTimer timer;
	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	QScopedPointer<QRegExp> ipAddr;
	if (ipAddrOut)
	{
		*ipAddrOut = 0;
		ipAddr.reset(new QRegExp("Connecting\\s+to\\s+(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+):(\\d+)", Qt::CaseInsensitive));
	}
	
	process.start(m_binaryMCat, args);

	if (!process.waitForStarted())
	{
		return false;
	}

	timer.start(timeoutMsec);

	while (process.state() != QProcess::NotRunning)
	{
		loop.exec();
		const bool bTimeOut = (!timer.isActive());
		while (process.canReadLine())
		{
			const QString line = QString::fromLatin1(process.readLine()).simplified();
			if (!ipAddr.isNull())
			{
				if (ipAddr->indexIn(line) >= 0)
				{
					quint32 values[4];
					if (MUtils::regexp_parse_uint32((*ipAddr), values, 4))
					{
						*ipAddrOut |= ((values[0] & 0xFF) << 0x18);
						*ipAddrOut |= ((values[1] & 0xFF) << 0x10);
						*ipAddrOut |= ((values[2] & 0xFF) << 0x08);
						*ipAddrOut |= ((values[3] & 0xFF) << 0x00);
					}
				}
			}
			log(line);
		}
		if (bTimeOut || MUTILS_BOOLIFY(m_cancelled))
		{
			qWarning("MCat process timed out <-- killing!");
			process.kill();
			process.waitForFinished();
			log(bTimeOut ? "!!! TIMEOUT !!!" : "!!! CANCELLED !!!");
			return false;
		}
	}

	timer.stop();
	timer.disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	if (process.exitCode() != 0)
	{
		log("Connection has failed!");
	}

	log(QString().sprintf("Exited with code %d", process.exitCode()), "");
	return (process.exitCode() == 0);
}

bool MUtils::UpdateChecker::checkSignature(const QString &file, const QString &signature)
{
	if (QFileInfo(file).absolutePath().compare(QFileInfo(signature).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		qWarning("CheckSignature: File and signature should be in same folder!");
		return false;
	}

	QString keyRingPath(m_binaryKeys);
	bool removeKeyring = false;
	if (QFileInfo(file).absolutePath().compare(QFileInfo(m_binaryKeys).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		keyRingPath = make_temp_file(QFileInfo(file).absolutePath(), "gpg");
		removeKeyring = true;
		if (!QFile::copy(m_binaryKeys, keyRingPath))
		{
			qWarning("CheckSignature: Failed to copy the key-ring file!");
			return false;
		}
	}

	QProcess process;
	init_process(process, QFileInfo(file).absolutePath());

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));

	process.start(m_binaryGnuPG, QStringList() << "--homedir" << "." << "--keyring" << QFileInfo(keyRingPath).fileName() << QFileInfo(signature).fileName() << QFileInfo(file).fileName());

	if (!process.waitForStarted())
	{
		if (removeKeyring)
		{
			remove_file(keyRingPath);
		}
		return false;
	}

	while (process.state() == QProcess::Running)
	{
		loop.exec();
		while (process.canReadLine())
		{
			log(QString::fromLatin1(process.readLine()).simplified());
		}
	}

	if (removeKeyring)
	{
		remove_file(keyRingPath);
	}

	log(QString().sprintf("Exited with code %d", process.exitCode()));
	return (process.exitCode() == 0);
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
