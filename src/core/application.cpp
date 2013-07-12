/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "application.h"
#include <typeinfo>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <boost/scoped_array.hpp>
#include <QEvent>
#include <QtDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QMetaType>
#include <QModelIndex>
#include <QSessionManager>
#include <QInputDialog>
#include <QSplashScreen>
#include <QProcess>
#include <QTimer>
#include <QCryptographicHash>
#include <QTextCodec>
#include <interfaces/isyncable.h>
#include <interfaces/ihaveshortcuts.h>
#include <util/util.h>
#include <util/structuresops.h>
#include <util/sys/paths.h>
#include "debugmessagehandler.h"
#include "tagsmanager.h"
#include "mainwindow.h"
#include "xmlsettingsmanager.h"
#include "core.h"
#include "coreinstanceobject.h"
#include "rootwindowsmanager.h"
#include "config.h"

#ifdef Q_OS_MAC
#include <QProxyStyle>
#include <QStyleFactory>
#endif

#ifdef Q_OS_WIN32
#include "winwarndialog.h"
#endif

namespace bpo = boost::program_options;

namespace LeechCraft
{
	Application::Application (int& argc, char **argv)
	: QApplication (argc, argv)
	, DefaultSystemStyleName_ (style ()->objectName ())
	, CatchExceptions_ (true)
	{
		Arguments_ = arguments ();
		bpo::options_description desc ("Allowed options");

		{
			std::vector<std::wstring> strings;
			for (const auto& arg : Arguments_.mid (1))
				strings.push_back (arg.toStdWString ());
			bpo::wcommand_line_parser parser (strings);
			VarMap_ = Parse (parser, &desc);
		}

		if (VarMap_.count ("help"))
		{
			std::cout << "LeechCraft " << LEECHCRAFT_VERSION << " (http://leechcraft.org)" << std::endl;
			std::cout << std::endl;
			std::cout << desc << std::endl;
			std::exit (EHelpRequested);
		}

		if (VarMap_.count ("version"))
		{
			std::cout << "LeechCraft " << LEECHCRAFT_VERSION << " (http://leechcraft.org)" << std::endl;
#ifdef Q_OS_WIN32
			std::cout << " <this version does not have UNLIMITED CAT POWA :(>" << std::endl;
#else
			std::cout << " this version can haz teh UNLIMITED CAT POWA :3 ε:" << std::endl;
#endif
			std::exit (EVersionRequested);
		}

		QTextCodec::setCodecForCStrings (QTextCodec::codecForName ("UTF-8"));

		if (VarMap_.count ("no-app-catch"))
			CatchExceptions_ = false;

		if (VarMap_.count ("clrsckt"))
			QLocalServer::removeServer (GetSocketName ());

		if (VarMap_.count ("no-resource-caching"))
			setProperty ("NoResourceCaching", true);

		if (VarMap_.count ("restart"))
		{
			Arguments_.removeAll ("--restart");
			EnterRestartMode ();
			return;
		}

		// Sanity checks
		if (!VarMap_.count ("plugin") && IsAlreadyRunning ())
			std::exit (EAlreadyRunning);

		Util::InstallTranslator ("", "qt", "qt4");

		QDir home = QDir::home ();
		if (!home.exists (".leechcraft"))
			if (!home.mkdir (".leechcraft"))
			{
				QMessageBox::critical (0,
						"LeechCraft",
						QDir::toNativeSeparators (tr ("Could not create path %1/.leechcraft")
							.arg (QDir::homePath ())));
				std::exit (EPaths);
			}

		// Things are sane, prepare
		QCoreApplication::setApplicationName ("Leechcraft");
		QCoreApplication::setApplicationVersion (LEECHCRAFT_VERSION);
		QCoreApplication::setOrganizationName ("Deviant");

		Translator_.reset (Util::InstallTranslator (""));

		qRegisterMetaType<QModelIndex> ("QModelIndex");
		qRegisterMetaType<QModelIndex*> ("QModelIndexStar");
		qRegisterMetaType<TagsManager::TagsDictionary_t> ("LeechCraft::TagsManager::TagsDictionary_t");
		qRegisterMetaType<Entity> ("LeechCraft::Entity");
		qRegisterMetaType<Entity> ("Entity");
		qRegisterMetaType<IHookProxy_ptr> ("LeechCraft::IHookProxy_ptr");
		qRegisterMetaType<Sync::ChainID_t> ("LeechCraft::Sync::ChainID_t");
		qRegisterMetaType<Sync::ChainID_t> ("Sync::ChainID_t");
		qRegisterMetaType<Sync::ChainID_t> ("ChainID_t");
		qRegisterMetaType<QKeySequences_t> ("QKeySequences_t");
		qRegisterMetaTypeStreamOperators<QKeySequences_t> ("QKeySequences_t");
		qRegisterMetaTypeStreamOperators<TagsManager::TagsDictionary_t> ("LeechCraft::TagsManager::TagsDictionary_t");
		qRegisterMetaTypeStreamOperators<Entity> ("LeechCraft::Entity");

		ParseCommandLine ();

#ifdef Q_OS_MAC
		if (!Arguments_.contains ("-nobundle"))
		{
			QDir dir (applicationDirPath ());
			dir.cdUp ();
			dir.cd ("PlugIns");
			QApplication::setLibraryPaths (QStringList (dir.absolutePath ()));
		}
#endif

		// Say hello to logs
		qDebug () << "======APPLICATION STARTUP======";
		qWarning () << "======APPLICATION STARTUP======";

#ifdef Q_OS_WIN32
		new WinWarnDialog;
#endif

		CheckStartupPass ();

		Core::Instance ();
		InitSettings ();

		InitPluginsIconset ();

		setWindowIcon (QIcon ("lcicons:/resources/images/leechcraft.svg"));

		setQuitOnLastWindowClosed (false);

		connect (this,
				SIGNAL (aboutToQuit ()),
				this,
				SLOT (handleQuit ()));

		Splash_ = new QSplashScreen (QPixmap (":/resources/images/macaroni.png"), Qt::SplashScreen);
		Splash_->show ();
		Splash_->setUpdatesEnabled (true);
		Splash_->showMessage (tr ("Initializing LeechCraft..."), Qt::AlignLeft | Qt::AlignBottom, QColor ("#FF3000"));

		connect (Core::Instance ().GetPluginManager (),
				SIGNAL (loadProgress (const QString&)),
				this,
				SLOT (handleLoadProgress (const QString&)));

		auto mw = Core::Instance ().GetRootWindowsManager ()->MakeMainWindow ();
		Core::Instance ().DelayedInit ();

		Splash_->showMessage (tr ("Finalizing..."), Qt::AlignLeft | Qt::AlignBottom, QColor ("#FF3000"));

		Splash_->finish (mw);
	}

	const QStringList& Application::Arguments () const
	{
		return Arguments_;
	}

	bpo::variables_map Application::Parse (bpo::wcommand_line_parser& parser,
			bpo::options_description *desc) const
	{
		bpo::variables_map vm;
		bpo::options_description invisible ("Invisible options");
		invisible.add_options ()
				("entity,E", bpo::wvalue<std::vector<std::wstring>> (), "the entity to handle");

		desc->add_options ()
				("help", "show the help message")
				("version,v", "print LC version")
				("download,D", "only choose downloaders for the entity: it should be downloaded but not handled")
				("handle,H", "only choose handlers for the entity: it should be handled but not downloaded")
				("type,T", bpo::value<std::string> (), "the type of the entity: url, url_encoded, file (for file paths) and such")
				("automatic", "the entity is a result of some automatic stuff, not user's actions")
				("bt", "print backtraces for warning messages into warning.log")
				("plugin,P", bpo::value<std::vector<std::string>> (), "load only given plugin and ignore already running instances of LC")
				("multiprocess,M", "load plugins in separate processes")
				("nolog", "disable custom file logger and print everything to stdout/stderr")
				("clrsckt", "clear stalled socket, use if you believe previous LC instance has terminated but failed to close its local socket properly")
				("no-app-catch", "disable exceptions catch-all in QApplication::notify(), useful for debugging purposes")
				("safe-mode", "disable all plugins so that you can manually enable them in Settings later")
				("list-plugins", "list all non-adapted plugins that were found and exit (this one doesn't check if plugins are valid and loadable)")
				("no-resource-caching", "disable caching of dynamic loadable resources (useful for stuff like Azoth themes development)")
				("autorestart", "automatically restart LC if it's closed (not guaranteed to work everywhere, especially on Windows and Mac OS X)")
				("minimized", "start LC minimized to tray")
				("restart", "restart the LC");
		bpo::positional_options_description pdesc;
		pdesc.add ("entity", -1);

		bpo::options_description all;
		all.add (*desc);
		all.add (invisible);

		bpo::store (parser
				.options (all)
				.positional (pdesc)
				.allow_unregistered ()
				.run (), vm);
		bpo::notify (vm);

		return vm;
	}

	const bpo::variables_map& Application::GetVarMap () const
	{
		return VarMap_;
	}

#ifdef Q_OS_WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

	QString Application::GetSocketName ()
	{
		QString templ = QString ("LeechCraft_local_socket_%1");
#ifdef Q_OS_WIN32
		boost::scoped_array<TCHAR> buffer (new TCHAR [0]);
		DWORD size = 0;
		GetUserName (buffer.get (), &size);
		buffer.reset (new TCHAR [size]);
		if (GetUserName (buffer.get (), &size))
#ifdef _UNICODE
			return templ.arg (QString::fromWCharArray (buffer.get ()));
#else
			return templ.arg (buffer.get ());
#endif
		else
			return templ.arg ("unknown");
#else
		return templ.arg (getuid ());
#endif
	}

	void Application::InitiateRestart ()
	{
		QStringList arguments = Arguments_;
		arguments.removeFirst ();
		arguments << "-restart";
		QProcess::startDetached (applicationFilePath (), arguments);

		qApp->quit ();
	}

	bool Application::notify (QObject *obj, QEvent *event)
	{
		if (event->type () == QEvent::LanguageChange)
			return true;

		if (CatchExceptions_)
		{
			try
			{
				return QApplication::notify (obj, event);
			}
			catch (const std::exception& e)
			{
				qWarning () << Q_FUNC_INFO
					<< QString::fromUtf8 (e.what ())
					<< typeid (e).name ()
					<< "for"
					<< obj
					<< event
					<< event->type ();
			}
			catch (...)
			{
				qWarning () << Q_FUNC_INFO
					<< obj
					<< event
					<< event->type ();
			}
			return false;
		}
		else
			return QApplication::notify (obj, event);
	}

	void Application::commitData (QSessionManager& sm)
	{
		if (Arguments_.contains ("-autorestart"))
			sm.setRestartHint (QSessionManager::RestartImmediately);

		sm.release ();
	}

	void Application::saveState (QSessionManager& sm)
	{
		commitData (sm);
	}

	bool Application::IsAlreadyRunning () const
	{
		QLocalSocket socket;
		socket.connectToServer (GetSocketName ());
		if (socket.waitForConnected () ||
				socket.state () == QLocalSocket::ConnectedState)
		{
			QDataStream out (&socket);
			out << Arguments_;
			if (socket.waitForBytesWritten ())
				return true;
			if (socket.error() == QLocalSocket::UnknownSocketError)
				return true;
		}
		else
		{
			switch (socket.error ())
			{
				case QLocalSocket::ServerNotFoundError:
				case QLocalSocket::ConnectionRefusedError:
					break;
				default:
					qWarning () << Q_FUNC_INFO
						<< "socket error"
						<< socket.error ();
					return true;
			}
		}

		// Clear any halted servers and their messages
		QLocalServer::removeServer (GetSocketName ());
		return false;
	}

	namespace
	{
		void Rotate (QDir lcDir, const QString& filename)
		{
			if (!lcDir.exists (filename))
				return;

			if (QFileInfo (lcDir.filePath (filename)).size () < 20 * 1024 * 1024)
				return;

			if (lcDir.exists (filename + ".0") &&
					!lcDir.remove (filename + ".0"))
				qWarning () << Q_FUNC_INFO
						<< "unable to remove old file"
						<< filename + ".0";

			if (!lcDir.rename (filename, filename + ".0"))
				qWarning () << Q_FUNC_INFO
						<< "failed to rename"
						<< filename
						<< "to"
						<< filename + ".0";
		}
	}

	void Application::ParseCommandLine ()
	{
		if (VarMap_.count ("nolog"))
		{
			qInstallMsgHandler (0);
			return;
		}

		if (VarMap_.count ("bt"))
			qInstallMsgHandler (DebugHandler::backtraced);
		else
			qInstallMsgHandler (DebugHandler::simple);

		QDir lcDir = QDir::home ();
		lcDir.cd (".leechcraft");

		Rotate (lcDir, "debug.log");
		Rotate (lcDir, "warning.log");
	}

	void Application::InitPluginsIconset ()
	{
		QDir::addSearchPath ("lcicons", ":/");

		const auto& pluginsIconset = XmlSettingsManager::Instance ()->
				property ("PluginsIconset").toString ();
		if (pluginsIconset == "Default")
			return;

		const auto& pluginsPath = "global_icons/plugins/" + pluginsIconset;
		for (const auto& cand : Util::GetPathCandidates (Util::SysPath::Share, pluginsPath))
			QDir::addSearchPath ("lcicons", cand);
	}

	void Application::EnterRestartMode ()
	{
		QTimer *timer = new QTimer;
		connect (timer,
				SIGNAL (timeout ()),
				this,
				SLOT (checkStillRunning ()));
		timer->start (1000);
	}

	void Application::CheckStartupPass ()
	{
		const auto& storedPass = XmlSettingsManager::Instance ()->property ("StartupPassword").toString ();
		if (storedPass.isEmpty ())
			return;

		const auto& pass = QInputDialog::getText (0,
				tr ("Startup password"),
				tr ("Enter startup password for LeechCraft:"),
				QLineEdit::Password);
		if (QCryptographicHash::hash (pass.toUtf8 (), QCryptographicHash::Sha1).toHex () != storedPass)
		{
			if (!pass.isEmpty ())
				QMessageBox::critical (0, "LeechCraft", tr ("Sorry, incorrect password"));
			std::exit (0);
		}
	}

	void Application::InitSettings ()
	{
		XmlSettingsManager::Instance ()->RegisterObject ("AppQStyle",
				this, "handleAppStyle");
		handleAppStyle ();

		XmlSettingsManager::Instance ()->RegisterObject ("Language",
				this, "handleLanguage");
		PreviousLangName_ = XmlSettingsManager::Instance ()->property ("Language").toString ();
	}

	void Application::handleQuit ()
	{
		Core::Instance ().Release ();
		XmlSettingsManager::Instance ()->Release ();
	}

#ifdef Q_OS_MAC
	namespace
	{
		class ToolbarFixerProxy : public QProxyStyle
		{
		public:
			ToolbarFixerProxy (QStyle *other)
			: QProxyStyle (other)
			{
			}

			int pixelMetric (PixelMetric metric, const QStyleOption *opt, const QWidget *w) const
			{
				auto result = baseStyle ()->pixelMetric (metric, opt, w);
				if (metric == PM_ToolBarIconSize)
					result = std::min (24, result);
				return result;
			}
		};
	}
#endif

	void Application::handleAppStyle ()
	{
		auto style = XmlSettingsManager::Instance ()->property ("AppQStyle").toString ();

		if (style == "Default")
			style = DefaultSystemStyleName_;

		if (style.isEmpty ())
		{
#ifdef Q_OS_WIN32
			style = "Plastique";
			XmlSettingsManager::Instance ()->setProperty ("AppQStyle", style);
#endif
		}

#ifdef Q_OS_MAC
		if (auto styleObj = QStyleFactory::create (style))
			setStyle (new ToolbarFixerProxy (styleObj));
#else
		setStyle (style);
#endif
	}

	void Application::handleLanguage ()
	{
		const auto& newLang = XmlSettingsManager::Instance ()->property ("Language").toString ();
		if (newLang == PreviousLangName_)
			return;

		PreviousLangName_ = newLang;

		if (QMessageBox::question (0,
					"LeechCraft",
					tr ("This change requires restarting LeechCraft. "
						"Do you want to restart now?"),
					QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;

		InitiateRestart ();
	}

	void Application::handleLoadProgress (const QString& str)
	{
		Splash_->showMessage (str, Qt::AlignLeft | Qt::AlignBottom, QColor ("#FF3000"));
	}

	void Application::checkStillRunning ()
	{
		if (IsAlreadyRunning ())
			return;

		QProcess::startDetached (applicationFilePath (), Arguments_);

		quit ();
	}
}
