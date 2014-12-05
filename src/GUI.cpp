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

#include <MUtils/GUI.h>

//Internal
#include "Utils_Win32.h"

//Qt
#include <QIcon>
#include <QApplication>
#include <QWidget>

///////////////////////////////////////////////////////////////////////////////
// BROADCAST
///////////////////////////////////////////////////////////////////////////////

bool MUtils::GUI::broadcast(int eventType, const bool &onlyToVisible)
{
	if(QApplication *app = dynamic_cast<QApplication*>(QApplication::instance()))
	{
		qDebug("Broadcasting %d", eventType);
		
		bool allOk = true;
		QEvent poEvent(static_cast<QEvent::Type>(eventType));
		QWidgetList list = app->topLevelWidgets();

		while(!list.isEmpty())
		{
			QWidget *widget = list.takeFirst();
			if(!onlyToVisible || widget->isVisible())
			{
				if(!app->sendEvent(widget, &poEvent))
				{
					allOk = false;
				}
			}
		}

		qDebug("Broadcast %d done (%s)", eventType, (allOk ? "OK" : "Stopped"));
		return allOk;
	}
	else
	{
		qWarning("Broadcast failed, could not get QApplication instance!");
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// WINDOW ICON
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace GUI
	{
		namespace Internal
		{
			class WindowIconHelper : public QObject
			{
			public:
				WindowIconHelper(QWidget *const parent, const HICON hIcon, const bool &bIsBigIcon)
				:
					QObject(parent),
					m_hIcon(hIcon)
				{
					SendMessage(parent->winId(), WM_SETICON, (bIsBigIcon ? ICON_BIG : ICON_SMALL), LPARAM(hIcon));
				}

				~WindowIconHelper(void)
				{
					if(m_hIcon)
					{
						DestroyIcon(m_hIcon);
					}
				}

			private:
				const HICON m_hIcon;
			};
		}
	}
}

bool MUtils::GUI::set_window_icon(QWidget *window, const QIcon &icon, const bool bIsBigIcon)
{
	if((!icon.isNull()) && window->winId())
	{
		const int extend = (bIsBigIcon ? 32 : 16);
		if(HICON hIcon = qicon_to_hicon(icon, extend, extend))
		{
			if(new Internal::WindowIconHelper(window, hIcon, bIsBigIcon))
			{
				return true;
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// FORCE QUIT
///////////////////////////////////////////////////////////////////////////////

void MUtils::GUI::force_quit(void)
{
	qApp->closeAllWindows();
	qApp->quit();
}

///////////////////////////////////////////////////////////////////////////////