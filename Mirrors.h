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

#include <cstdlib>

/*
 * Update mirrors, for automatic update check
 */
static const char *update_mirrors[] =
{
	"http://muldersoft.chickenkiller.com/",
	"http://muldersoft.com/",
	"http://mulder.bplaced.net/",
	"http://mulder.000webhostapp.com/",		//"http://mulder.webuda.com/", //"http://mulder.6te.net/",
	"http://mulder.pe.hu/",
	"http://muldersoft.square7.ch/",
	"http://muldersoft.co.nf/",				//"http://muldersoft.eu.pn/",
	"http://muldersoft.lima-city.de/",
	"http://www.muldersoft.keepfree.de/",
	"http://lamexp.sourceforge.net/",
	"http://muldersoft.sourceforge.net/",
	"http://lamexp.osdn.io/",
	"http://x264-launcher.osdn.io/",
	"http://lordmulder.github.io/LameXP/",
	"http://muldersoft.bitbucket.io/",
	"http://www.tricksoft.de/",
	"http://repo.or.cz/LameXP.git/blob_plain/gh-pages:/",
	"http://gitlab.com/lamexp/lamexp/raw/gh-pages/",
	NULL
};

/*
 * List of known hosts, for connectivity test
 */
static const char *known_hosts[] =		//Taken form: http://www.alexa.com/topsites !!!
{
	"www.163.com",
	"www.7-zip.org",
	"www.ac3filter.net",
	"clbianco.altervista.org",
	"status.aws.amazon.com",
	"build.antergos.com",
	"www.aol.com",
	"www.apache.org",
	"www.apple.com",
	"www.adobe.com",
	"archive.org",
	"www.artlebedev.ru",
	"web.audacityteam.org",
	"status.automattic.com",
	"www.avidemux.org",
	"www.babylon.com",
	"www.baidu.com",
	"bandcamp.com",
	"www.bbc.co.uk",
	"www.berlios.de",
	"www.bing.com",
	"www.bingeandgrab.com",
	"www.bucketheadpikes.com",
	"www.buzzfeed.com",
	"www.cam.ac.uk",
	"www.ccc.de",
	"home.cern",
	"www.citizeninsomniac.com",
	"www.cnet.com",
	"cnzz.com",
	"www.cuhk.edu.hk",
	"www.codeplex.com",
	"www.codeproject.com",
	"www.der-postillon.com",
	"www.ebay.com",
	"www.equation.com",
	"www.ethz.ch",
	"www.farbrausch.de",
	"fc2.com",
	"fedoraproject.org",
	"blog.fefe.de",
	"www.ffmpeg.org",
	"blog.flickr.net",
	"www.fraunhofer.de",
	"free-codecs.com",
	"git-scm.com",
	"doc.gitlab.com",
	"www.gmx.net",
	"news.gnome.org",
	"www.gnu.org",
	"go.com",
	"code.google.com",
	"haali.su",
	"www.harvard.edu",
	"www.heise.de",
	"www.helmholtz.de",
	"www.huffingtonpost.co.uk",
	"www.hu-berlin.de",
	"www.iana.org",
	"www.imdb.com",
	"www.imgburn.com",
	"imgur.com",
	"www.iuj.ac.jp",
	"www.jd.com",
	"www.jiscdigitalmedia.ac.uk",
	"kannmanumdieuhrzeitschonnbierchentrinken.de",
	"mirrors.kernel.org",
	"komisar.gin.by",
	"lame.sourceforge.net",
	"www.libav.org",
	"blog.linkedin.com",
	"www.linuxmint.com",
	"www.livedoor.com",
	"www.livejournal.com",
	"longplayer.org",
	"go.mail.ru",
	"marknelson.us",
	"www.mediafire.com",
	"web.mit.edu",
	"www.mod-technologies.com",
	"ftp.mozilla.org",
	"www.mpg.de",
	"mplayerhq.hu",
	"www.msn.com",
	"wiki.multimedia.cx",
	"www.nch.com.au",
	"neocities.org",
	"mirror.netcologne.de",
	"oss.netfarm.it",
	"www.netflix.com",
	"blog.netflix.com",
	"netrenderer.de",
	"www.nytimes.com",
	"www.opera.com",
	"osdn.net",
	"www.oxford.gov.uk",
	"www.ox-fanzine.de",
	"www.partha.com",
	"pastebin.com",
	"pastie.org",
	"portableapps.com",
	"www.portablefreeware.com",
	"posteo.de",
	"support.proboards.com",
	"www.qq.com",
	"www.qt.io",
	"www.quakelive.com",
	"rationalqm.us",
	"www.reddit.com",
	"www.rwth-aachen.de",
	"www.seamonkey-project.org",
	"selfhtml.org",
	"www.sina.com.cn",
	"www.sohu.com",
	"help.sogou.com",
	"sourceforge.net",
	"www.spiegel.de",
	"www.sputnikmusic.com",
	"stackoverflow.com",
	"www.stanford.edu",
	"www.t-online.de",
	"www.tagesschau.de",
	"tdm-gcc.tdragon.net",
	"www.tdrsmusic.com",
	"tu-dresden.de",
	"www.ubuntu.com",
	"portal.uned.es",
	"www.unibuc.ro",
	"www.uniroma1.it",
	"www.univ-paris1.fr",
	"www.univer.kharkov.ua",
	"www.univie.ac.at",
	"www.uol.com.br",
	"www.uva.nl",
	"www.uw.edu.pl",
	"www.videohelp.com",
	"www.videolan.org",
	"virtualdub.org",
	"blog.virustotal.com",
	"www.vkgoeswild.com",
	"www.warr.org",
	"www.weibo.com",
	"status.wikimedia.org",
	"www.winamp.com",
	"wpde.org",
	"x265.org",
	"xhmikosr.1f0.de",
	"xiph.org",
	"us.mail.yahoo.com",
	"www.youtube.com",
	"www.zedo.com",
	"ffmpeg.zeranoe.com",
	NULL
};
