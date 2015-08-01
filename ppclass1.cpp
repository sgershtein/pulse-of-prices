//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// program classes file no. 2 (because the first one exceeded 64K limit)
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 10-Dec-95
//-----------------------------------------------------------------------------

#include "pp.h"	      // main header file
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <alloc.h>
#include <math.h>
#include <io.h>
#include <except.h>
#include <fstream.h>
#pragma hdrstop
#include "dcode.h";

//#define KEYREADY 9202601l  // ONLY DEFINE THIS TO DISABLE KEY CHECKING!

#ifdef KEYREADY
#define WARNCIC
#endif

//#define DEBUG

extern PPI ppi;
extern PPF ppf;
extern PPD ppd;
extern PPG ppg;

extern int cmps(char *s1, char *s2);

void drawuser() {
	char userstr[100];
   strcpy(userstr,"Распространение запрещено. Лицензия на имя ");
	strcat(userstr,basekey.username);
	Display d;
	Attr at;
	Mouse m;
	m.hide();
	VPutsColorMid(40,d.yMax()-1,userstr,at[attrUserName]);
	m.show();
}

/****************************** DataWindow ********************************/

BOOL DataWindow::ChewMenuCommand(char c) {
   long l;
   switch (c) {
      case cmSelect:
         select(lcur,!selected(lcur)); // reverse selection status
         down();
         return TRUE;
      case cmSelectAll:
         for (l=0; l<lno; l++)
            select(l,TRUE);
			draw();
         return TRUE;
      case cmDeselectAll:
         for (l=0; l<lno; l++)
            select(l,FALSE);
         draw();
         return TRUE;
      case cmSelReverse:
         for (l=0; l<lno; l++)
            select(l,!selected(l)); // reverse selection status
			draw();
         return TRUE;
	}
	return FALSE;
}

void DataWindow::draw() { // show number of elements/current/number of selected
	ListWindow::draw();
	char s[20];
	static ml=0;
	sprintf(s," %ld/%ld ",lcur+1,lno);
	if (ml>strlen(s))
		VSetChar(x2()-ml-1,y1(),ml,frame[4]);
	ml=strlen(s);
	VPuts(x2()-ml-1,y1(),s);

	static mls=0;
	static long nsel0=0;
	if (nsel!=nsel0 || redraw) {
		nsel0=nsel;
		sprintf(s," Отмечено: %ld ",nsel);
		if (mls>strlen(s) || nsel==0) {
			VSetColor(x2()-mls-1,y2(),mls,attr[attrListWin]);
			VSetChar(x2()-mls-1,y2(),mls,frame[4]);
		}
		mls=strlen(s);
		if (nsel) {
			VPuts(x2()-mls-1,y2(),s);
			VSetColor(x2()-mls-1,y2(),mls,attr[attrListWinSel]);
		}
	}
}
/**************************** BaseInfoBox *********************************/

BaseInfoBox::BaseInfoBox() : savebuf(NULL),
   ControlWindow(Rect(5,4,75,19),"",F_DOUBLE,TRUE,TRUE) {
   if (display.yMax()>25) {
      rect.y1+=(display.yMax()-24)/2;
      rect.y2+=(display.yMax()-24)/2;
   }
   AddControl(new Button("  OK  ",0,(cl()-6)/2,ch()-2,ncOkButton,0,TRUE));
   draw();
   GoModal("Информация о базе данных...");
   Restore(savebuf);
}

void BaseInfoBox::draw() {
   Save(savebuf);
   mouse.hide();
   Statusbar st;
   ControlWindow::draw();
   VPutsMid((x1()+x2())/2,y1()," Информация ");
   char s[70];
   char ss[]="Не найден файл базы pulse.pp%c! Проверьте настройку каталогов.";
   BOOL opg=ppg.isopen();
   BOOL opd=ppd.isopen();
   BOOL opf=ppf.isopen();
   BOOL opi=ppi.isopen();
   Setup setup;
   if (!opg)
      if (!ppg.open(setup.datapath)) {
         sprintf(s,ss,'g');
         VPutsColorMid((cx1()+cx2())/2,cy1()+3,s,
            (attr[attrcode()] & 0xf0) | attr[attrRED]);
         mouse.show();
         return;
		}
	if (!opd)
      if (!ppd.open(setup.datapath)) {
         sprintf(s,ss,'d');
         VPutsColorMid((cx1()+cx2())/2,cy1()+3,s,
            (attr[attrcode()] & 0xf0) | attr[attrRED]);
         mouse.show();
         return;
		}
   if (!opi)
      if (!ppi.open(setup.datapath)) {
         sprintf(s,ss,'i');
         VPutsColorMid((cx1()+cx2())/2,cy1()+3,s,
            (attr[attrcode()] & 0xf0) | attr[attrRED]);
         mouse.show();
         return;
      }
   if (!opf)
      if (!ppf.open(setup.datapath)) {
         sprintf(s,ss,'f');
         VPutsColorMid((cx1()+cx2())/2,cy1()+3,s,
            (attr[attrcode() & 0xf0]) | attr[attrRED]);
         mouse.show();
         return;
      }
   Date d=ppd.dcreate;
   sprintf(s,"Дата создания базы данных: %02d.%02d.%d",
      d.day(),d.month(),d.yy());
   char l=1;
   VPuts(cx1()+5,cy1()+l++,s);
/*   d=ppd.dupdate;
   sprintf(s,"Дата последнего обновления базы данных: %02d.%02d.%d",
      d.day(),d.month(),d.yy());
   VPuts(cx1()+5,cy1()+l++,s);
   sprintf(s,"Количество обновлений базы данных: %d",ppd.nupdat);
   VPuts(cx1()+5,cy1()+l++,s); */
	long nsup, ndem;
   ppi.iselect(iSupply);
   nsup = ppi.RecNo();
   ppi.iselect(iDemand);
   ndem = ppi.RecNo();
   sprintf(s,"Записей в базе данных: %lu",nsup+ndem);
   VPuts(cx1()+5,cy1()+l++,s);
   sprintf(s,"В том числе спрос: %lu, предложение: %lu",ndem,nsup);
   VPuts(cx1()+5,cy1()+l++,s);
   sprintf(s,"Фирм в базе данных: %u",ppf.Nfirms());
   VPuts(cx1()+5,cy1()+l++,s);
   sprintf(s,"Количество используемых форм оплаты: %u",ppi.npaym);
   VPuts(cx1()+5,cy1()+l++,s);
   sprintf(s,"Суммарный размер файлов базы, килобайт: %u",
      (ppi.fsize()+ppd.fsize()+ppf.fsize()+ppg.fsize())/1024);
   VPuts(cx1()+5,cy1()+l++,s);
   long lk;
   sprintf(s,"Необходимое место для временного файла, килобайт: %lu",
      lk=((nsup+ndem)/16+(nsup+ndem)*8)/1024);
   VPuts(cx1()+5,cy1()+l++,s);
   struct diskfree_t free;
   long avail;
   char drive=upcase(setup.temppath[0]);
   if (!_dos_getdiskfree(drive-'A'+1, &free)) { // no error
      avail = (long) free.avail_clusters
            * (long) free.bytes_per_sector
				* (long) free.sectors_per_cluster;
		sprintf(s,"На диске %c: свободно, килобайт: %lu",
			drive,
			avail/1024);
		VPuts(cx1()+5,cy1()+l,s);
		if (avail/1024>lk)
			VPutsColor(cx1()+5+strlen(s),cy1()+l++," - достаточно.",
				/*(*/attr[attrcode()]/* & 0xf0) | attr[attrGREEN]*/);
		else
			VPutsColor(cx1()+5+strlen(s),cy1()+l++," - не достаточно!",
				(attr[attrcode()] & 0xf0) | attr[attrRED]);
	} else {
		sprintf(s,
			"Ошибка на диске %c - невозможно создать временный файл!",drive);
      VPutsColor(cx1()+5,cy1()+l++,s,
         (attr[attrcode()] & 0xf0) | attr[attrRED]);
   }
   Date today;
   today.Today(); // set today's date.
   int n;
   sprintf(s,"Дней с момента последнего обновления: %d",n=today-d);
   VPuts(cx1()+5,cy1()+l++,s);
   if (n>setup.expirydays)
      VPutsColor(cx1()+5,cy1()+l++,
         "База устарела - пора позаботиться о новой версии!",
         (attr[attrcode()] & 0xf0) | attr[attrRED]);
   if (!opg)
      ppg.close();
	if (!opd)
		ppd.close();
	if (!opf)
		ppf.close();
	if (!opi)
		ppi.close();
	mouse.show();
}

/**************************** KeyCheck *********************************/

/*char KeyCheck::key[5];
char *KeyCheck::keypath;*/

/*long KeyCheck::DecodeKey(char key[5]) {
	// decoding
	char kk[4];
	kk[0]=key[1]; kk[1]=key[2]; kk[2]=key[3]; kk[3]=key[4];

	unsigned *up=(unsigned*)kk;
	*up=DCODE(*up);	up++;
	*up=DCODE(*up);

	// removing solt
	kk[3] ^= ~kk[0];
	kk[2] ^= kk[0];
	kk[1] ^= ~key[0];
	kk[0] ^= key[0];

#ifdef DEBUG
	printf("decoded KEY: %03d%03d-%03d%03d\n",kk[1],kk[2],kk[0],kk[3]);
#endif
	long *lp=(long*)kk;
	return *lp;
} */

long KeyCheck::getCIC() {
	unsigned char CIC[17] = "Pulse of Prices.\0";	// Computer Identifier Code
	long *lp=(long*)CIC;
	if ( *((char*)0xf000fff7l)=='/' && *((char*)0xf000fffal)=='/') {
		*lp++ = *((long*)0xf000fff5l);  		  	// f000:fff5 - first half of date
		*lp++ = *((long*)0xf000fff9l);  		  	// f000:fff9 - second half of date
	} else {
		cerr << "BIOS DATE NOT FOUND\n"; //throw 1.1;
	}
	if ( *((unsigned short*)0xc0000000l) == 0xAA55 ) {
		*lp++ = *((long*)0xc0000100l);  		  	// c000:0100 - video adapter data
		*lp   = *((long*)0xc0000110l);  		  	// c000:0110 - some more
	} else {
		cerr << "VIDEO ADAPTER NOT FOUND\n"; //throw 1.1;
	}

/*	*lp++ = *((long*)0xc0000100l);  		  	// c000:0100 - video adapter data
	*lp++ = *((long*)0xc0000110l);  		  	// c000:0110 - some more
	long *int42=(long*)*((long*)0x104l);	// 0000:0104 - first HDD table addr
	*lp   =  *int42++;
	*lp   |= *int42++;
	*lp++ ^= *int42++;
	long *bios=(long*)*((long*)0xf00ffee0l);	// f000:fee0 - BIOS
	*lp	= *bios++;
	*lp	^= *bios++ & CIC[0];
	*lp	^= *bios++ & CIC[2];
	*lp	^= *bios++ & CIC[4];
	*lp	^= *bios++ & CIC[6];
	*lp	^= *bios++ & CIC[8];
	*lp	^= *bios++ & CIC[12];
	*lp	^= *bios++ & CIC[14];*/

	CIC[0] += CIC[16]; CIC[1] += CIC[15];
	CIC[2] += CIC[14]; CIC[3] += CIC[13];
	CIC[4] += CIC[12]; CIC[5] += CIC[11];
	CIC[6] += CIC[10]; CIC[7] += CIC[9];

	lp = (long*)CIC;
	lp[0] ^= lp[1];

	return *lp;
}

BOOL KeyCheck::warnCIC() { // display CIC information on screen
#ifdef WARNCIC // display warning about CIC on screen
	char CIC[4];
	long *pCIC=(long*)CIC;
	*pCIC = getCIC();
	char s[50];
	randomize();
	char solt=random(255);
	// adding solt
	CIC[0] ^= solt;
	CIC[1] ^= ~solt;
	CIC[2] ^= CIC[0];
	CIC[3] ^= ~CIC[0];
	unsigned *up=(unsigned*)CIC;
	*up=DCODE(*up);	up++;
	*up=DCODE(*up);
	short sum = 0;
	for(int is=0; is<4; is++) {
		sum += CIC[is]/100 + CIC[is]/10 + CIC[is];
	}
	sum += solt/100 + solt/10 + solt;
	sum%=10;
	if (sum==0)
		sum = 10;
	sprintf(s,"%1d%03d-%03d%03d-%03d%03d",10-sum,
		solt,CIC[1],CIC[2],CIC[0],CIC[3]);

	char buf[100]=" Сообщите код в Урал Релком: ";
	strcat(buf,s);
	MessageBox(" Код этого компьютера (CIC) ",buf,
	  "Подробная информация о CIC в файле PULSEDOC.TXT");
	return TRUE;
#else
	return FALSE;
#endif
}

void KeyCheck::printCIC() {
	char CIC[4];
	long *pCIC=(long*)CIC;
	*pCIC = getCIC();
	char s[50];

#ifdef DEBUG
	printf("decoded CIC without solt: %03d%03d-%03d%03d",CIC[1],CIC[2],CIC[0],CIC[3]);
#endif

	randomize();
	char solt=random(255);

	// adding solt
	CIC[0] ^= solt;
	CIC[1] ^= ~solt;
	CIC[2] ^= CIC[0];
	CIC[3] ^= ~CIC[0];

	unsigned *up=(unsigned*)CIC;
	*up=DCODE(*up);	up++;
	*up=DCODE(*up);

	short sum = 0;
	for(int is=0; is<4; is++) {
		sum += CIC[is]/100 + CIC[is]/10 + CIC[is];
	}
	sum += solt/100 + solt/10 + solt;

	sum%=10;
	if (sum==0)
		sum = 10;

	sprintf(s,"%1d%03d-%03d%03d-%03d%03d",10-sum,solt,CIC[1],CIC[2],CIC[0],CIC[3]);

	cerr << "\nИдентификационный код компьютера (CIC): ";
	cout << s << endl;
}

void KeyCheck::FindKey() {
#ifdef KEYREADY // the key is not in the base!
	basekey.set(KEYREADY);
	strcpy(basekey.username,"--none--");
#else

	char sNOKEY[]="\n\n"
	"Подробно о механизме защиты с использованием ключей Вы можете\n"
	"прочитать в файле PULSEDOC.TXT.  Этот механизм введен с целью ограничения\n"
	"незаконного распространения Пульса Цен. Со всеми вопросами (в частности,\n"
	"и о приобретении Пульса Цен) Вы можете обращаться в Урал Релком:\n\n"
	"        Тел.   (3432) 59-87-21, 58-04-40.\n"
	"        Факс   (3432) 59-49-56.\n"
	"        Email  sg@mplik.ru (технические вопросы)\n"
	"               nadegda@mplik.ru (вопросы подписки и получения ключей)\n"
	"        Адрес  620219, Екатеринбург, ул. А. Валека 13 офис 401\n\n"
	"МЫ ЦЕНИМ ВАШЕ ЖЕЛАНИЕ ПРИОБРЕСТИ ПУЛЬС ЦЕН, А НЕ КОПИРОВАТЬ ЕГО НЕЗАКОННО.\n"
	"ПРИНОСИМ СВОИ ИСКРЕННИЕ ИЗВИНЕНИЯ ЗА ВЫНУЖДЕННЫЕ НЕУДОБСТВА\n";

	if (basekey.getkey()>=0) // key already set!
		return;

	BOOL opg=ppg.isopen();
	Setup setup;
	if (!opg)
		if (!ppg.open(setup.datapath)) {
			cerr << "*** НЕ НАЙДЕНЫ ФАЙЛЫ БАЗЫ ДАННЫХ ***" << sNOKEY;
			printCIC();
#ifndef NOKEYTHROW
			throw 1.1;
#endif
		}

//	long CIC=getCIC();								// this computer CIC
	basekey.set(ppg.getKEY(getCIC()));
	if (basekey.getkey()<0) {
		cerr << "** ФАЙЛЫ БАЗЫ ДАННЫХ НЕ СОДЕРЖАТ КЛЮЧА ДЛЯ ВАШЕГО КОМПЬЮТЕРА **\n" <<
		"Пульс цен не может начать работу" << sNOKEY;
		printCIC();
#ifndef NOKEYTHROW
		throw 1.1;
#endif
	}
#endif // KEYREADY
}

/**************************** Setup *********************************/

long Setup::freemem=coreleft();         // initial free memory in bytes
int Setup::easyexit;
int Setup::grmousecur;
int Setup::userpal;
int Setup::hires;
int Setup::lefthandmouse;
int Setup::showbrief;
int Setup::roubleprice;
int Setup::expirydays;
char Setup::temppath[70];
char Setup::datapath[70];
char *Setup::inipath;
int Setup::frewrite;
int Setup::fshort;
int Setup::bchar;
char Setup::filterpath[70];


char *inistr[] = {"GraphMouseCursor",
		"UserPalette","HiResolution","LeftHandMouse","ShowBrief",
      "RoublePrice","TempPath","ExpiryDays","DataPath","FOverWrite",
      "FShortForm","BChar","FilterPath","EasyExit"};

BOOL Setup::read(Code code) {   // read setup
	Attr attr;
	if (code==all) {
		for (int i=all+1; i<LAST; i++)
			read((Code)i);
	}
	IniFile *ini=new IniFile(inipath,"pulse"); // open .ini file
	if (!ini->Is_open()) {
		delete ini;
		ini = new IniFile(inipath,"puls");
	}
	if (code==all) { // reading user defined color scheme
		char secname[]="UserPalette";
		for (int i=0; i<=attrMax; i++) {
			char s[4];
			sprintf(s,"%d",i);
			int tmp=attr(i);
			ini->GetInt(secname,s,tmp);
			attr(i)=tmp;
		}
	} else {
		char secname[]="Settings";
		switch (code) {
			case BChar:
				bchar = 32;
				ini->GetInt(secname,inistr[code-1],bchar);
				break;
			case FOverWrite:
				frewrite = FALSE;
				ini->GetInt(secname,inistr[code-1],frewrite);
				break;
         case FShortForm:
            fshort = 0;
            ini->GetInt(secname,inistr[code-1],fshort);
            break;
         case EasyExit:
            easyexit = 1;
            ini->GetInt(secname,inistr[code-1],easyexit);
            break;
         case grMouseCur:
            grmousecur = TRUE;
            ini->GetInt(secname,inistr[code-1],grmousecur);
            break;
         case UserPal: {
            userpal = scCheck;
            ini->GetInt(secname,inistr[code-1],userpal);
            Attr attr;
				if (userpal!=scCheck)
               attr.Scheme(userpal);
            break;
         }
         case HiRes:
            hires=FALSE;
				ini->GetInt(secname,inistr[code-1],hires);
            break;
         case LeftHandMouse:
            lefthandmouse=FALSE;
            ini->GetInt(secname,inistr[code-1],lefthandmouse);
            break;
         case ShowBrief:
            showbrief=TRUE;
            ini->GetInt(secname,inistr[code-1],showbrief);
            break;
         case RoublePrice:
            roubleprice=FALSE;
            ini->GetInt(secname,inistr[code-1],roubleprice);
            break;
         case DataPath:
            datapath[0]=0;
            ini->GetStr(secname,inistr[code-1],datapath,69);
            if (datapath[0]==0)
               strcpy(datapath,inipath);
            break;
         case FilterPath:
            filterpath[0]=0;
				ini->GetStr(secname,inistr[code-1],filterpath,69);
            if (filterpath[0]==0)
               strcpy(filterpath,datapath);
            break;
         case TempPath:
				temppath[0]=0;
            ini->GetStr(secname,inistr[code-1],temppath,65);
				if (temppath[0]==0)
               strcpy(temppath,getenv("TEMP"));
            if (temppath[0]==0)
               strcpy(temppath,getenv("TMP"));
            if (temppath[strlen(temppath)-1]!='\\')
               strcat(temppath,"\\");
            if (temppath[1]!=':') {
               char tt[70]="C:";
               strcat(tt,temppath);
               strcpy(temppath,tt);
            }
            break;
         case ExpiryDays:
            expirydays=14;
            ini->GetInt(secname,inistr[code-1],expirydays);
            break;
      }
   }
   delete ini;
   return TRUE;
}

/*BOOL Setup::save(Code code) {   // save setup
   if (code==all) {
      for (Code i=(Code)(all+1); i<LAST; i++)
         save(i);
   }
//   IniFile ini(inipath,"pulse"); // open .ini file
   IniFile *ini=new IniFile(inipath,"pulse"); // open .ini file
	if (!ini->Is_open()) {
      delete ini;
      ini = new IniFile(inipath,"puls");
   }
   char secname[]="Settings";
   switch (code) {
      case BChar:
         ini->PutInt(secname,inistr[code-1],bchar);
         break;
      case FOverWrite:
         ini->PutInt(secname,inistr[code-1],frewrite);
         break;
      case FShortForm:
         ini->PutInt(secname,inistr[code-1],fshort);
         break;
      case grMouseCur:
         ini->PutInt(secname,inistr[code-1],grmousecur);
         break;
      case UserPal:
         ini->PutInt(secname,inistr[code-1],userpal);
			break;
      case HiRes:
         ini->PutInt(secname,inistr[code-1],hires);
         break;
      case LeftHandMouse:
         ini->PutInt(secname,inistr[code-1],lefthandmouse);
         break;
		case ShowBrief:
         ini->PutInt(secname,inistr[code-1],showbrief);
			break;
      case RoublePrice:
         ini->PutInt(secname,inistr[code-1],roubleprice);
         break;
      case TempPath:
         ini->PutStr(secname,inistr[code-1],temppath);
         break;
      case DataPath:
         ini->PutStr(secname,inistr[code-1],datapath);
         break;
      case FilterPath:
         ini->PutStr(secname,inistr[code-1],filterpath);
         break;
      case ExpiryDays:
         ini->PutInt(secname,inistr[code-1],expirydays);
         break;
   }
   delete ini;
   return TRUE;
}  */

//------------------------------ AboutBox ------------------------------------

AboutBox::AboutBox(BOOL b) : savebuf(NULL), timeout(b),
   ControlWindow(Rect(11,5,68,17+INTERNAL),"",F_DOUBLE,TRUE,TRUE) {
   if (display.yMax()>25) {
      rect.y1+=(display.yMax()-24)/2;
		rect.y2+=(display.yMax()-24)/2;
   }
	AddControl(new Button("  OK  ",0,(cl()-6)/2,ch()-2,ncOkButton,0,TRUE));
   draw();
   if (!timeout)
      GoModal();
   else {
      volatile unsigned long *time=(unsigned long*)0x46c;
      static unsigned long tstart = *time;
      EventQueue eq;
      while (*time-tstart<18*3) {
         Event *e=eq.get();
         if (!e)
            continue;
         if (ChewEvent(e) && ncode) {
            delete e;
            break;
         }
			delete e;
      }
   }
   Restore(savebuf);
}

void AboutBox::draw() {
   Save(savebuf);
   mouse.hide();
   ControlWindow::draw();

	VPutsMid((x1()+x2()+1)/2,y1()+2,
      "Электронный Пульс Цен");
	char verstr[60];
   sprintf(verstr,"Версия %s (%s)",VERSION,__DATE__);
   VPutsMid((x1()+x2()+1)/2,y1()+3,verstr);
#if INTERNAL
   VPutsColorMid((x1()+x2()+1)/2,y1()+4,
      "ДЛЯ ВНУТРЕННЕГО ТЕСТИРОВАНИЯ В УРАЛ-РЕЛКОМ",
      (attr[attrcode()] & 0xf0)+YELLOW+BLINK);
#endif
#ifdef DEMO
   VPutsColorMid((x1()+x2()+1)/2,y1()+4,
      "ДЕМОНСТРАЦИОННАЯ ВЕРСИЯ",
      (attr[attrcode()] & 0xf0)+YELLOW+BLINK);
#endif
   VPutsMid((x1()+x2()+1)/2,y1()+5+INTERNAL,
      "Автор программы: Сергей Герштейн (sg@mplik.ru)");
   VPutsMid((x1()+x2()+1)/2,y1()+6+INTERNAL,
      "Экранная библиотека Валерия Филатова");
   VPutsMid((x1()+x2()+1)/2,y1()+8+INTERNAL,
		"Copyright (c) 1995-97, Ural Relcom, Ltd");
	mouse.show();
	mouse.moveto((x1()+x2()+1)/2,y1()+10+INTERNAL);
}

BOOL AboutBox::ChewEvent(Event *e) {
   if (ControlWindow::ChewEvent(e))
      return TRUE;
   else if (timeout && (e->type==evKeyPress || e->type==evMouseDown)) {
      ncode = ncCloseButton;
      return TRUE;
   }
   if (!timeout)
      return FALSE;
	volatile unsigned long *time=(unsigned long*)0x46c;
   static unsigned long tstart = *time;
	if (*time-tstart>18*3) { // some 10 seconds
		ncode = ncCloseButton;
		return TRUE;
	}
	return FALSE;
}

//------------------------------ RelcomInfo------------------------------------

RelcomInfo::RelcomInfo() : savebuf(NULL),
	ControlWindow(Rect(13,3,65,21),"",F_DOUBLE,TRUE,TRUE) {
	if (display.yMax()>25) {
		rect.y1+=(display.yMax()-24)/2;
		rect.y2+=(display.yMax()-24)/2;
	}
	AddControl(new Button("  OK  ",0,(cl()-6)/2,ch()-2,ncOkButton,0,TRUE));
	draw();
	GoModal();
	Restore(savebuf);
}

void RelcomInfo::draw() {
	Save(savebuf);
	mouse.hide();
	ControlWindow::draw();
	int yy=1;
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"Электронный Пульс Цен");
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"эксклюзивно распространяется");
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"компанией Урал-Релком");
	yy++;
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"620219, Екатеринбург, ул. А. Валека, 13, офис 401");
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"Телефоны: (3432) 59-87-21, 59-49-56 (факс)");
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"Email: postmaster@mplik.ru, sg@mplik.ru");
	yy++;
	VPutsColorMid((x1()+x2()+1)/2,y1()+yy++,
		"По другим адресам - только ПИРАТСКИЕ копии,",
		(attr[attrcode()] & 0xf0) | attr[attrYELLOW]);
	VPutsColorMid((x1()+x2()+1)/2,y1()+yy++,
		"устаревшие, содержащие вирусы и ошибки.",
		(attr[attrcode()] & 0xf0) | attr[attrYELLOW]);
	VPutsColorMid((x1()+x2()+1)/2,y1()+yy++,
		"Не поощряйте пиратов, обратитесь к нам!",
		(attr[attrcode()] & 0xf0) | attr[attrRED]);
	yy++;
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"По вопросам подачи объявлений в базу данных");
	VPutsMid((x1()+x2()+1)/2,y1()+yy++,"обращайтесь в фирму АБАК (22-30-46, 51-90-61)");
	mouse.show();
}

//---------------------------- Filters stuff ----------------------------------

Filter::~Filter() {
	AndCondition *cur=head;
	while (cur) {
      cur=head->next;
      delete head;
      head=cur;
   }
   head=NULL;
   ncond=0;
}

BOOL Filter::operator()(char *ad, int fcode, float price) {
// check if given record passes thru the filter.  TRUE is yes.
   AndCondition *cur=head;
   if (isand) {
      while (cur)
         if (!(*cur)(ad,fcode,price))
            return FALSE;
         else
            cur=cur->next;
      return TRUE;
   }
   while (cur)
      if ((*cur)(ad,fcode,price))
         return TRUE;
      else
         cur=cur->next;
   return FALSE;
}

void Filter::insert(AndCondition *here, AndCondition *cond) {
// insert new condition before *here into the list
   AndCondition *cur=head;
   if (!head) {
      head=cond;
      cond->next=NULL;
      ncond++;
      return;
   }
   cond->next=here;
   if (here==head) {
      head=cond;
      ncond++;
      return;
   }
   while (cur && cur->next!=here)
      cur = cur->next;
   if (!cur)
      throw 1;    // element not found
   cur->next=cond;
   ncond++;
}

void Filter::remove(AndCondition *cond) {
   if (head==cond) {
      head=cond->next;
      delete cond;
      ncond--;
      return;
   }
   AndCondition *cur=head;
   while (cur && cur->next!=cond)
      cur=cur->next;
   if (!cur)
      throw 1; // element not found
   cur->next=cond->next;
   delete cond;
   ncond--;
}

AndCondition::~AndCondition() {
   OrCondition *cur=head;
   while (cur) {
      cur=head->next;
      delete head;
      head=cur;
   }
   ncond=0;
   head=NULL;
}

BOOL AndCondition::operator()(char *ad, int fcode, float price) {
// check if given record passes thru the condition.  TRUE is yes.
   OrCondition *cur=head;
   if (isand) {
      while (cur)
         if (!(*cur)(ad,fcode,price))
            return FALSE;
         else
            cur=cur->next;
      return TRUE;
   }
   while (cur)
      if ((*cur)(ad,fcode,price))
         return TRUE;
      else
         cur=cur->next;
   return FALSE;
}

void AndCondition::insert(OrCondition *here, OrCondition *cond) {
// insert new condition before *here into the list
   OrCondition *cur=head;
   if (!head) {
      head=cond;
      cond->next=NULL;
      ncond++;
      return;
   }
   cond->next=here;
   if (here==head) {
      head=cond;
      ncond++;
      return;
   }
   while (cur && cur->next!=here)
      cur = cur->next;
   if (!cur)
      throw 1;    // element not found
   cur->next=cond;
   ncond++;
}

void AndCondition::remove(OrCondition *cond) {
// remove this condition from the list
   if (head==cond) {
      head=cond->next;
      delete cond;
      ncond--;
      return;
   }
   OrCondition *cur=head;
   while (cur && cur->next!=cond)
      cur=cur->next;
   if (!cur)
      throw 1; // element not found
   cur->next=cond->next;
   delete cond;
   ncond--;
}


BOOL FilAd::operator()(char *ad, int, float) {
// check if given ad string complies with the filter
   switch (ctype) {
      case contains:
         return strstr(ad,str)!=NULL;
      case ncontains:
         return strstr(ad,str)==NULL;
   }
// this point must never be reached
   MessageBox(" Внутренняя ошибка! ","FilAd::operator()");
   throw 1.1;
}

BOOL FilFirm::operator()(char *, int fcode, float) {
// check if given firm complies with the filter
   if (lct!=ctype) {
      if (bf)
         delete bf;
      bf=NULL;
      lct=ctype;
   }
   if (bf && bf[fcode/8]&(2<<((fcode%8)*2))) { // checking in the table
      return (bf[fcode/8]>>((fcode%8)*2)) & 1;
   }
   BOOL fo=ppf.isopen();
   Setup setup;
   BOOL ret=TRUE; // value to be returned
   if (!fo)
      if (!ppf.open(setup.datapath)) {
         MessageBox(" Ой ","Не могу открыть файл pulse.ppf");
         return TRUE;
      }
   if (!bf) {
      bf=new unsigned int[ppf.Nfirms()/8+1];
      _fmemset(bf,0,ppf.Nfirms()/4+1);
   }
   if (!ppf.readn(fcode,TRUE)) {
      MessageBox(" Ой ","Ошибка чтения файла pulse.ppf");
      return TRUE;
   }
   switch (ctype) {
      case contains:
         ret=(strstr(ppf.firm,str)!=NULL); break;
      case ncontains:
         ret=(strstr(ppf.firm,str)==NULL); break;
      case equal:
         if (ppf.firm[0]==' ' && str[0]!=' ')
            ret=(strcmp(ppf.firm+1,str)==0);
         else
            ret=(strcmp(ppf.firm,str)==0); break;
      case nequal:
         if (ppf.firm[0]==' ' && str[0]!=' ')
            ret=(strcmp(ppf.firm+1,str)!=0);
         else
            ret=(strcmp(ppf.firm,str)!=0); break;
   }
   if (!fo)
      ppf.close();
   bf[fcode/8]|=(2+ret)<<((fcode%8)*2);
   return ret;
}

BOOL FilPrice::operator()(char *, int, float price) {
// check if given price complies with the filter
   switch (ctype) {
      case equal:
         return abs(price-value)<0.001;   // all these 0.001 are for handling
      case nequal:                        // rounding errors
         return abs(price-value)>0.001;
      case nless:
         return price>=(value-0.001);
      case less:
         return price<value;
      case ngreater:
         return price<=(value+0.001);
      case greater:
         return price>value;
   }

// this point must never be reached
   MessageBox(" Внутренняя ошибка! ","FilPrice::operator()");
   throw 1.1;
}

// ------------------------- FilterWindow -----------------------------------


// rect, number of elements, title, frame, shadow, closebutton, parent
FilterWindow::FilterWindow() : Filter(), chkvalid(FALSE),
   ListWindow(Rect(0,1,79,20),1," Редактирование фильтра ",
   F_SINGLE,FALSE,TRUE) {
   Display d;
   if (d.yMax()>24)
      rect=Rect(0,1,79,d.yMax()-4);
   AddControl(bnew=new Button(" Очистить ",2,33,y2()-1,ncNewButton,
   A_ALT_J,FALSE));
   strcpy(filname,/*"* без названия *"*/ "* * *");

#ifdef TESTFILTER // this defined in options!!!
// temporary from here ---------------------
   insert(head,new AndCondition(AndCondition::filFirm));
   insert(head,new AndCondition(AndCondition::filPrice));
   char *s1=new char[61];
   strcpy(s1,"nequal");
   head->next->insert(head->next->head,new FilFirm(OrCondition::nequal,s1));
   s1=new char[61];
   strcpy(s1,"contains");
   head->next->insert(head->next->head,new FilFirm(OrCondition::contains,s1));
   head->insert(head->head,new FilPrice(OrCondition::nless,10.5));
   countlno();
// temporary till here ---------------------
#endif
}

void FilterWindow::countlno() { // count number of lines
   lno=1+ncond;
   if (ncond>1)
      lno++;
   for (AndCondition *cur=head; cur; cur=cur->next)
      lno+=cur->ncond+(cur->ncond>1?1:0);
   resize(rect);
   if (lcur>=lno)
      moveto(lno-1);
}

FilterWindow::~FilterWindow() {
   // nothing right now
}

BOOL FilterWindow::operator[](long ofs) {
   // check if this offset complies with the filter
   if (!chkvalid) { // first find out which fields should be checked
      chkfirm=chkprice=chkad=FALSE;
      AndCondition *ac=head;
      while (ac) {
         switch (ac->field) {
            case AndCondition::filFirm: chkfirm=TRUE; break;
            case AndCondition::filPrice: chkprice=TRUE; break;
            case AndCondition::filAd: chkad=TRUE; break;
         }
         ac=ac->next;
      }
   }  // done validating fields to check.
   if (!ppd.read(ofs,chkad)) {
      MessageBox(" FilterWindow::operator[] ","Ошибка чтения файла pulse.ppd");
      return FALSE;
   }

   Rates r;

   return (*this)(ppd.drec.ad,ppd.drec.fcode,ppd.drec.price*r[ppd.drec.pcode]);
}

void FilterWindow::draw() {
   mouse.hide();
   char rd=redraw;
   if (rd==255) {
      VClrFrame(x1(),y2(),cl(),2,' ',attr[attrcode()]);
   }
   ListWindow::draw();
   if (rd==255) {
      VFrame(x1(),y2(),cl(),2,frame,attr[attrcode()]);
   }
   VPutch(x1(),y2(),195);
   VPutch(x2(),y2(),180);
   mouse.show();
}

char* FilterWindow::getline(long n) {     // get n-th line
   char buf[80];
   if (n==0) {
      strcpy(buf,"Фильтр: ");
      return strcat(buf,filname);
   }
   AndCondition *cur=head;
   int curline=1;
   if (ncond>1)
      if (n==1)  // main "and"/"or" line
         if (isand)
            return " (И)";
         else
            return "(ИЛИ)";
      else
         curline++;
   buf[0]=buf[1]=buf[3]=' ';
   if (isand)
      buf[2]=179; // single vertical line (for and)
   else
      buf[2]=186; // double vertical line (for or)
   buf[1]=buf[3]=' ';
   buf[4]=0;
   while (cur) {
      if (n==curline) { // AndCondition's line
         if (isand) {
            if (cur->next)
               buf[2]=195; // |-
            else
               buf[2]=192;
            buf[3]=196; // -
         } else {
            if (cur->next)
               buf[2]=204; // ||=
            else
               buf[2]=200;
            buf[3]=205; // =
         }
         switch (cur->field) {
            case AndCondition::filFirm:
               return strcat(buf,"ФИРМА");
            case AndCondition::filAd:
               return strcat(buf,"ОБЪЯВЛЕНИЕ");
            case AndCondition::filPrice:
               return strcat(buf,"ЦЕНА");
         }
      }

      if (cur->ncond>1)
         curline++;

      if (!cur->next)
         buf[2]=' ';

      if (n==curline)  // either "or" or "and" strings
         if (cur->isand)
            return strcat(buf," (И)");
         else
            return strcat(buf,"(ИЛИ)");

      if (n>curline+cur->ncond) { // shall we skip next group?
         curline++;
         curline+=cur->ncond;
         cur=cur->next;
         continue;
      }

// now checking inside...
      OrCondition *cur1=cur->head;
      curline++;
      while (cur1) {
         if (n==curline) {// OrCondition's line
            buf[4]=buf[5]=' ';
            buf[8]=0;
            if (cur->isand) {
               if (cur1->next)
                  buf[6]=195; // |-
               else
                  buf[6]=192;
               buf[7]=196; // -
            } else {
               if (cur1->next)
                  buf[6]=204; // ||=
               else
                  buf[6]=200;
               buf[7]=205; // =
            }
            switch (cur1->ctype) {
               case OrCondition::equal: strcat(buf," равно "); break;
               case OrCondition::nequal: strcat(buf," не равно "); break;
               case OrCondition::less: strcat(buf," меньше "); break;
               case OrCondition::nless: strcat(buf," не меньше "); break;
               case OrCondition::greater: strcat(buf," больше "); break;
               case OrCondition::ngreater: strcat(buf," не больше "); break;
               case OrCondition::contains: strcat(buf," содержит "); break;
               case OrCondition::ncontains: strcat(buf," не содержит "); break;
            }
            switch (cur->field) {
               case AndCondition::filFirm: {
                  strcat(buf,"\x22");
                  FilFirm *ff=(FilFirm*)cur1;
                  strcat(buf,ff->getstr());
                  return strcat(buf,"\x22");
               }
               case AndCondition::filAd: {
                  strcat(buf,"\x22");
                  FilAd *ff=(FilAd*)cur1;
                  strcat(buf,ff->getstr());
                  return strcat(buf,"\x22");
               }
               case AndCondition::filPrice: {
                  char ss[20];
                  FilPrice *ff=(FilPrice*)cur1;
                  sprintf(ss,"%.2f",ff->getvalue());
                  return strcat(buf,ss);
               }
            }
         }  // curline found
         curline++;
         cur1=cur1->next;
      }  // while inside

// this point must never be reached
      MessageBox(" Внутренняя ошибка! ","FilterWindow::getline() point 1");
      throw 1.1;
   }
// this point must never be reached
   MessageBox(" Внутренняя ошибка! ","FilterWindow::getline() point 2");
   throw 1.1;
}

void FilterWindow::insor(OrCondition *here,AndCondition *ac) {
   // checking memory
   try {
      void *trial=new char[30000];
      delete[] trial;
   } catch (xalloc) {
      MessageBox(" Извините ",
        "Не хватает памяти для добавления нового условия");
      return;
   }
   // end checking memory
   char *s=new char[61];
   OrCondition *oc;
   s[0]=0;
   switch (ac->field) {
      case AndCondition::filFirm:
         ac->insert(here,oc=new FilFirm(OrCondition::contains,s));
         break;
      case AndCondition::filPrice:
         delete s;
         ac->insert(here,oc=new FilPrice(OrCondition::nless,0));
         break;
      case AndCondition::filAd:
         ac->insert(here,oc=new FilAd(OrCondition::contains,s));
         break;
   }
   chor(oc,ac->field);
}

void FilterWindow::chor(OrCondition *here,AndCondition::filtype type) {
   Display d;
   Statusbar sbar;
   char *sold=sbar.get();
   {  // ************* here controlwindow cw exist
      ControlWindow cw(Rect(9,d.yMax()/2-4,71,d.yMax()/2+3+
         (type==AndCondition::filFirm?1:0)),
         " Введите новое условие: ",F_DOUBLE,TRUE,TRUE);
      RadioButton rb0[]={
         RadioButton(2,3,"Содержит",6,3),
         RadioButton(2,4,"Не содержит",6,4)};
      RadioButton rb1[]={
         RadioButton(2,3,"Содержит",6,3),
         RadioButton(2,4,"Не содержит",6,4),
         RadioButton(2,5,"Равно",6,5),
         RadioButton(2,6,"Не равно",6,6)};
      RadioButton rb2[]={
         RadioButton(10,0,"Равно",14,0),
         RadioButton(10,1,"Не равно",14,1),
         RadioButton(10,2,"Больше",14,2),
         RadioButton(10,3,"Не больше",14,3),
         RadioButton(10,4,"Меньше",14,4),
         RadioButton(10,5,"Не меньше",14,5)};
      char *temps;
      int num;   // number of active button
      switch (type) {
         case AndCondition::filPrice: {
            FilPrice *fa=(FilPrice*)here;
            temps=new char[11];
            sprintf(temps,"%.2f",fa->getvalue());
            switch (fa->ctype) {
               case OrCondition::equal: num=0; break;
               case OrCondition::nequal: num=1; break;
               case OrCondition::greater: num=2; break;
               case OrCondition::ngreater: num=3; break;
               case OrCondition::less: num=4; break;
               case OrCondition::nless: num=5; break;
            }
            break;
         }
         case AndCondition::filFirm: {
            FilFirm *fa=(FilFirm*)here;
            temps=fa->getstr();
            switch (fa->ctype) {
               case OrCondition::contains: num=0; break;
               case OrCondition::ncontains: num=1; break;
               case OrCondition::equal: num=2; break;
               case OrCondition::nequal: num=3; break;
            }
            break;
         }
         case AndCondition::filAd: {
            FilAd *fa=(FilAd*)here;
            temps=fa->getstr();
            switch (fa->ctype) {
               case OrCondition::contains: num=0; break;
               case OrCondition::ncontains: num=1; break;
            }
            break;
         }
      }
      RadioButtonGroup *rbg=new RadioButtonGroup(
         type==AndCondition::filAd?rb0:type==AndCondition::filFirm?rb1:rb2,
         type==AndCondition::filAd?2:type==AndCondition::filFirm?4:6,num,
         "Используйте стрелки для выбора поля.");
      InputField *ib=(type==AndCondition::filPrice?
         new InputField(34,1,10,temps,InputField::it_float,
         "Введите новое значение. Для установки условий нажмите <TAB>."):
         new InputField(1,1,59,temps,
         (InputField::InpType)(InputField::it_any + InputField::it_caps),
         "Введите новое значение. Для установки условий нажмите <TAB>."));
      Button *b=new Button("   OK   ",0,35,4,ncOkButton,0,TRUE);
      b->SetTTLRUD(ib,rbg);
      ib->SetTTLRUD(rbg,b);
      rbg->SetTTLRUD(b,ib);
      void *buf=NULL;
      cw.Save(buf);
      cw.AddControl(b);
      cw.AddControl(rbg);
      cw.AddControl(ib);
      cw.draw();
      cw.GoModal();
      if (type==AndCondition::filPrice) {
         char *s;
         float f=strtod(temps,&s);
         FilPrice *fa=(FilPrice*)here;
         switch (rbg->selected()) {
            case 0: fa->ctype=OrCondition::equal; break;
            case 1: fa->ctype=OrCondition::nequal; break;
            case 2: fa->ctype=OrCondition::greater; break;
            case 3: fa->ctype=OrCondition::ngreater; break;
            case 4: fa->ctype=OrCondition::less; break;
            case 5: fa->ctype=OrCondition::nless; break;
         }
         if (!*s)
            fa->setvalue(f);
         delete temps;
      } else { // Ad and Firm are all the same
         switch (rbg->selected()) {
            case 0: here->ctype=OrCondition::contains; break;
            case 1: here->ctype=OrCondition::ncontains; break;
            case 2: here->ctype=OrCondition::equal; break;
            case 3: here->ctype=OrCondition::nequal; break;
         }
      }
      cw.Restore(buf);
   }
   sbar.say(sold);
   countlno();
   repaint(1);
}

void FilterWindow::insand(AndCondition *here) {
   // insert new AndCondition entered by user

   // check if there's enough memory
   try {
      void *trial=new char[30000];
      delete[] trial;
   } catch (xalloc) {
      MessageBox(" Извините ",
        "Не хватает памяти для добавления нового условия");
      return;
   }
   // end checking memory

   Display d;
   ControlWindow cw(Rect(23,d.yMax()/2-2,57,d.yMax()/2+2),
      " Добавить условие на поле: ",F_DOUBLE,TRUE,TRUE);
   RadioButton rb[]={
      RadioButton(3,0,"Объявление",7,0),
      RadioButton(3,1,"Фирма",7,1),
      RadioButton(3,2,"Цена",7,2)};
   RadioButtonGroup *rbg=new RadioButtonGroup(rb,3,0,
      "Используйте стрелки для выбора поля.");
   Button *b=new Button("   OK   ",0,22,1,ncOkButton,0,TRUE);
   rbg->SetTTLRUD(b,b);
   b->SetTTLRUD(rbg,rbg);
   cw.AddControl(b);
   cw.AddControl(rbg);
   void *buf=0;
   cw.Save(buf);
   cw.draw();
   if (cw.GoModal()==ncOkButton) {
      AndCondition *ac=new AndCondition((AndCondition::filtype)rbg->selected());
      insert(here,ac);
      char *temps=new char[61];
      temps[0]=0;
      switch (ac->field) {
         case AndCondition::filFirm:
            ac->insert(ac->head,new FilFirm(OrCondition::contains,temps));
            break;
         case AndCondition::filPrice:
            ac->insert(ac->head,new FilPrice(OrCondition::nless,0));
            delete temps;
            break;
         case AndCondition::filAd:
            ac->insert(ac->head,new FilAd(OrCondition::contains,temps));
            break;
      }
   }
   cw.Restore(buf);
   countlno();
   repaint(1);
}

BOOL FilterWindow::doedit(int n,ACTION act) {
   if (!act) return FALSE;
   chkvalid=FALSE;
   Statusbar sbar;
//   char *sold=sbar.get();
   Display d;
   // find a line for operations:
   if (n==0)  // do stuff with filter name
      switch (act) {// removing all the filter
         case del:
            ncode=ncNewButton;
            return TRUE;
         case ret:/* {// ret - change filter name
               void *buf=NULL;
               char *temps=new char[33];
               strcpy(temps,filname);
               ControlWindow cw(Rect(23,d.yMax()/2-3,57,d.yMax()/2+3),
                  " Новое название фильтра: ",F_DOUBLE,TRUE,TRUE);
               InputField *ib=new InputField(1,1,30,temps,InputField::it_any,
                  "Введите новое название.  Если передумали - нажмите <ESC>");
               // name, sel.char.pos, x,y , ncode, hotkey, def, saying, parent
               Button *b=new Button(" OK ",2,14,3,ncOkButton,0,TRUE);
               cw.AddControl(b);
               cw.AddControl(ib);
               cw.Save(buf);
               cw.draw();
               if (cw.GoModal()==ncOkButton)
                  strcpy(filname,temps);
               delete[] temps;
               cw.Restore(buf);
            }
            sbar.say(sold);
            repaint(1);
            return TRUE;  */
         case ins: // ins - insert new AddCondition
            insand(head);  // insert new andcondition before head
            return TRUE;
      }
   AndCondition *cur=head;
   int curline=1;
   if (ncond>1)
      if (n==1)  // main "and"/"or" line
         switch (act) {
            case del:
               return FALSE;
            case ret:  // ret - change
               isand=!isand;
               repaint(1);
               return TRUE;
            case ins:  // ins - insert new AddCondition
               insand(head);
               return TRUE;
         }
      else
         curline++;
   while (cur) {
      if (n==curline) // AndCondition's line
         switch (act) {
            case del: // delete addcondition
               remove(cur);
               countlno();
               repaint(1);
               return TRUE;
            case ret: // can't change addcondition
               beep();
               return TRUE;
            case ins: // insert new addcondition
               insand(cur);   // insert new andcondition before cur
               return TRUE;
         }
      if (cur->ncond>1)
         curline++;
      if (n==curline)  // either "or" or "and" strings (local)
         switch (act) {
            case del: return FALSE;
            case ret: // change condition
               cur->isand=!cur->isand;
               repaint(1);
               return TRUE;
            case ins: // insert new orcondition
               insor(cur->head,cur);
               return TRUE;
         }
      if (n>curline+cur->ncond) { // shall we skip next group?
         curline++;
         curline+=cur->ncond;
         cur=cur->next;
         continue;
      }
// now checking inside...
      OrCondition *cur1=cur->head;
      curline++;
      while (cur1) {
         if (n==curline) // OrCondition's line
            switch (act) {
               case del: // delete OrCondition
                  if (cur->ncond<2)
                     remove(cur);
                  else
                     cur->remove(cur1);
                  countlno();
                  repaint(1);
                  return TRUE;
               case ret:  // ret - change
                  chor(cur1,cur->field);
                  return TRUE;
               case ins:  // ins - insert new OrCondition
                  insor(cur1,cur);
                  return TRUE;
            }
         curline++;
         cur1=cur1->next;
      } // inside while
// this point must never be reached
      MessageBox(" Внутренняя ошибка! ","FilterWindow::doedit() point 1");
      throw 1.1;
   } // while
// this point must never be reached
   MessageBox(" Внутренняя ошибка! ","FilterWindow::doedit() point 2");
   throw 1.1;
}

BOOL FilterWindow::ChewEvent(Event *e) {
   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      if (lcur==0 && k->keycode>=' ') {
         return doedit(0,ret);
      }
      if (k->scancode==S_TAB) {
         if (!focus) {
            bnew->GotFocus();
            SetFocus(bnew);
         } else {
            bnew->LostFocus();
            SetFocus(NULL);
         }
         return TRUE;
      }
      if (!focus)
         switch (k->scancode) {
            case S_RET: return doedit((int)lcur,ret);
            case S_DEL: return doedit((int)lcur,del);
            case S_INS: return doedit((int)lcur,ins);
         }
   } else {
      MouseEvent *m=(MouseEvent*)e;
      if (m->type==evMouseDblClick && m->x>0 && m->x<79 &&
         m->y>=cy1() && m->y<=cy2())
            return doedit((int)lcur,ret);

   }
   return ListWindow::ChewEvent(e);
}

void FilterWindow::edit() {
   EventQueue eq;
   Cursor c;
   Statusbar st;
   char *stold=st.get();
   st.say("<INS> - добавить; <DEL> - удалить; ─┘ - изменить; <ESC> - выход.");
   int x=c.x(), y=c.y();
   c.hide();
   repaint(255);
   Event *e=NULL;
   ncode=0;
   while (ncode!=ncCloseButton) {
      e=eq.wait();
      if (!ChewEvent(e))
         if (e->type==evMouseDown) {
            MouseEvent *m=(MouseEvent*)e;
            if (m->y==0)
                beep();
         } else if (e->type==evKeyPress)
            beep();
      delete e;
      if (ncode==ncNewButton) {
         ClickSnd();
         ncode=0;
         Filter::~Filter();
         strcpy(filname,/*"* без названия *"*/"* * *");
         lno=1; lcur=lfirst=0;
         repaint(1);
      }
   }
   st.say(stold);
   c.moveto(x,y);
}

/*void FilterWindow::FileName(char *fname) {
   Setup s;
   strcpy(fname,s.filterpath);
   int l=strlen(fname);
   if (fname[l-1]!='\\' && fname[l-1]!=':')
      strcat(fname,"\\");
   strcat(fname,"filters.pp");
}

const char filtersig[5]="SGft";

BOOL FilterWindow::erase(unsigned ofs) { // remove from file filter on given ofs
   char s[100];
   FileName(s);
   fstream f(s,ios::in | ios::out | ios::binary);
   if (!f)
      return FALSE;
   long sig;
   f.read((char*)&sig,4);
   if (sig!=(long)*filtersig) {
      f.close();
      return FALSE;
   }
   f.seekg(ofs);
   int dlen;   // length of filter to delete
   f.read((char*)&dlen,2);
   char *buf;
   try {
     buf=new char[5000];
   } catch (xalloc) {
      f.close();
      return FALSE;
   }
   BOOL done=FALSE;
   do {
      f.seekg(ofs+dlen);
      f.read(buf,5000);
      f.seekp(ofs);
      if (f.eof()) {
         f.clear();
         done=TRUE;
      }
      f.write(buf,f.gcount());
      ofs+=5000;
   } while (!done);
   delete[] buf;
   f.close();
   return TRUE;
}

BOOL FilterWindow::write() {              // save filter to disk
// MUST CHECK for filter with the same name and delete it if necessary
   try {
      char s[100];
      FileName(s);
      fstream f(s,ios::in | ios::out | ios::binary);
      if (!f)
         throw 1;
      f.seekg(0,ios::end);
      if (f.tellg()==0) {  // just created new file!
         f.clear();
         f.write(filtersig,4); // storing signature
      } else {             // file already exists
         long sig;  // checking signature
         f.seekg(0);
         f.read((char*)&sig,4);
         if (sig!=(long)*filtersig) {
            f.close();
            throw 1; // wrong signature
         }
         // checking if we need to delete old filter with the same name

         f.seekp(0,ios::end); // getting ready to store the filter
      }
      // now storing the filter

      f.close();
      { MessageBox(" Фильтр сохранен ",
        "Введенный Вами фильтр сохранен на диске.",
        "Нажмите <Enter> для продолжения."); }
      return TRUE;
   } catch (int) {
      MessageBox(" Ошибка записи фильтра ",
        "Файл фильров не может быть открыт или поврежден",
        "Возможно с файлом сейчас работают другиме пользователями в сети");
      return FALSE;
   }
}

void FilterWindow::manage() {             // manage saved filters

   beep(); beep(); beep();
#pragma message
}

BOOL read(unsigned ofs) {  // read filter from disk from given ofs

#pragma message
}

  */

// ***************************** Rates ****************************************

BOOL Rates::valid=FALSE;   // are the rates valid
float *Rates::rates=NULL; // the rates.
BOOL *Rates::user=NULL;   // are these user rates?
int Rates::npaym=0;

Rates::Rates() {
   if (!valid)
      ReadRates();
}

BOOL Rates::ReadRates() {
   Setup setup;
   BOOL opi=ppi.isopen();
	if (!opi)
      if (!ppi.open(setup.datapath))
         return FALSE;
   npaym=ppi.npaym;
   if (valid) {
      delete[] rates;
      delete[] user;
   }
   rates = new float[npaym];
   user = new BOOL[npaym];
   for (int i=0; i<npaym; i++) {
      user[i]=FALSE;
      rates[i]=ppi.paym[i].coef;
   }
   if (!opi)
      ppi.close();
   return valid=TRUE;
}

void Rates::clearuser(int i) {  // clear user rate for i
   if (!valid)
      return;
   Setup setup;
   BOOL opi=ppi.isopen();
   if (!opi)
      if (!ppi.open(setup.datapath))
         return;
   user[i]=FALSE;
   rates[i]=ppi.paym[i].coef;
   if (!opi)
      ppi.close();
}

// ************************** FirmWindow *************************************

FirmWindow::FirmWindow(TopMenu *mt) : tpos(0), sel(NULL), oldcur(-1),
      drinfo(2),
      DataWindow(mt,Rect(1,2,3,4),0," Фирмы ",F_DOUBLE,TRUE,TRUE) {
   Display d;
   Setup setup;
   if (!ppf.open(setup.datapath)) { // error opening file!
      beep();
      MessageBox mb(" Ой ","Ошибка чтения файла данных pulse.ppf",
         "Файл не найден или поврежден...");
   } else {
      // rect, number of elements, title, frame, shadow, closebutton, parent
      lno=ppf.Nfirms();
      sel=new unsigned[(int)(lno/16+1)];
      memset(sel, 0, (int)(lno/16+1)*2); // initialize the array with zeros
      resize(Rect(35-15,2,44+15,d.yMax()-8));
      mtop->SetChecked(TRUE,cmFirms);
      mtop->SetDisabled(FALSE,cmSelect);
      mtop->SetDisabled(FALSE,cmSelectAll);
      mtop->SetDisabled(FALSE,cmDeselectAll);
      mtop->SetDisabled(FALSE,cmSelReverse);
      draw();
      statusbar.say(
         "Выберите интересующие Вас фирмы и нажмите ─┘");
   }
}

FirmWindow::~FirmWindow() {
   if (sel) {
      delete[] sel;
      mtop->SetChecked(FALSE,cmFirms);
      mtop->SetDisabled(TRUE,cmSelect);
      mtop->SetDisabled(TRUE,cmSelectAll);
      mtop->SetDisabled(TRUE,cmDeselectAll);
      mtop->SetDisabled(TRUE,cmSelReverse);
      Cursor c;
      c.hide();
   }
}

BOOL FirmWindow::selected(long n) {
   if (!sel)
      return FALSE;
   int pos = (int)n>>4;  //n/16;
   int ofs = (int)n&15; //n%16;
   return ((sel[pos] >> ofs) & 1)==1;
}

void FirmWindow::select(long n, BOOL b) {    // select or deselect n-th line
   if (!sel)
      return;
   int pos = (int)n>>4; //n/16;
   int ofs = (int)n&15; //n%16;
   unsigned bit;
   bit = 1 << ofs;
   if (selected(n)!=b)
      nsel+=b?1:-1;
   if (b)
      sel[pos] |= bit;
   else
      sel[pos] &= ~bit;
   if (visible(n))
      repaint(1);
}

BOOL FirmWindow::ChewEvent(Event *e) {     // should handle <Enter> key
   if (e->type==evMouseDblClick) {
      MouseEvent *m=(MouseEvent*)e;
      if (m->x>=cx1() && m->x<=cx2() && m->y<=cy2() && m->y>=cy1()) {
         if (!nsel) // if nothing is selected => select current
            select((int)lcur,TRUE);
         EventQueue eq;
         eq.setcommand(caOpenBaseWin);
         return TRUE;
      }
   }
   if (e->type==evKeyPress) {
      drinfo=10;
      KeyEvent *k=(KeyEvent*)e;
      if (k->scancode==S_RET) {
         if (!nsel) // if nothing is selected => select current
            select((int)lcur,TRUE);
         EventQueue eq;
			eq.setcommand(caOpenBaseWin);
         return TRUE;
      }
      if ((k->scancode==S_BS || k->scancode==S_LEFT) && tpos>0) {
         tpos--;
         repaint(1);
         return TRUE;
      }
      if (k->keycode>=' ') { // regular character pressed
         char ch=upcase(k->keycode);
         char s[60];
         char *s0=getline(lcur);
         for (char c=0; c<tpos; c++)
            s[c]=s0[c];
         s[c++]=ch; s[c]=0;
         // will binary search for the string;
         long left=0, right=lno-1, mid=lcur;
         while (left<=right) {
            int cmp=cmps(s,getline(mid)); // compare strings until first ends
            if (cmp>0)
               left=mid+1;
            else if (cmp<0)
               right=mid-1;
            else {// found
               while (mid>=left && (mid==0 || !cmps(s,getline(mid-1))))
                  mid--;
               tpos++;
               moveto(mid);
               return TRUE;
            }
            mid=(left+right)/2;
         }
         // not found...
         beep();
      }
   }
   long lc=lcur;
   if (DataWindow::ChewEvent(e)) {
      if (ncode==ncCloseButton) { // close the window
         EventQueue eq;
         eq.setcommand(caCloseDataWin);
      }
      if (lc!=lcur) {
         tpos=0;
         repaint(1);
      }
      drinfo=10;
      return TRUE;
   } else
      return FALSE;
}

BOOL FirmWindow::ChewMenuCommand(char cm) { // return TRUE if command chown

   return DataWindow::ChewMenuCommand(cm);
}

void FirmWindow::draw() {                // draw the window
   char rep=redraw;
   DataWindow::draw();
   { // drawing the address window
      mouse.hide();
      if (rep==255) {
         VClrFrame(0,y2()+3,78,3,' ',attr[attrAddrInfoF]);
         VFrame(0,y2()+3,78,3,F_SINGLE,attr[attrAddrInfoF]);
      }
      if (rep || (lcur!=oldcur && drinfo==1)) { // need to reoutput the window
         drinfo=0;
         oldcur=lcur;
         if (ppf.readn((short)lcur)) {
            char str[3][80];
            if (!ppf.fullname || strlen(ppf.fullname)<2)
               sprintf(str[0],"Фирма: %s",ppf.firm);
            else {
               int ii=77-9-strlen(ppf.firm);
               if (strlen(ppf.fullname)>ii)
                  ppf.fullname[ii]=0;
               sprintf(str[0],"Фирма: %s, %-s",ppf.firm,ppf.fullname);
            }
            char s[15];
            sprintf(str[1],"Адрес: %s",ppf.address);
            sprintf(str[2],"Телефоны: (%03lu) ",ppf.area);
            for (int i=0; i<4; i++)
               if (ppf.phone[i]) {
                  if (i==3)
                     strcat(str[2],", Факс: ");
                  else if (i)
                     strcat(str[2],", ");
                  sprintf(s,"%7lu",ppf.phone[i]);
                  char ss[10];
                  ss[0]=s[0]; ss[1]=s[1]; ss[2]=s[2]; ss[3]='-';
                  ss[4]=s[3]; ss[5]=s[4]; ss[6]='-';
                  ss[7]=s[5]; ss[8]=s[6]; ss[9]=0;
                  strcat(str[2],ss+(ppf.phone[i]>999999l?0:1));
               }
            for (i=0; i<3; i++) {
               int j=(i==2?9:6);
               VPutsColorLen(2+j,rect.y2+4+i,str[i]+j,
                  attr[attrAddrDataF],76-j);
               VPutsLen(2,rect.y2+4+i,str[i],j);
            }
         }
      }
      mouse.show();
   }
   Cursor c;
   c.moveto(cx1()+1+tpos,cy1()+(int)(lcur-lfirst));
}

char* FirmWindow::getline(long n) {      // return n-th line
   if (!ppf.readn((short)n,TRUE)) {
      beep();
      MessageBox bm(" Ой ","Ошибка чтения файла данных pulse.ppf",
         "Файл не найден или поврежден...");
      return "";
   }
   return ppf.firm;
}


