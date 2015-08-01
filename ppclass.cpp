//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// program classes file
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 11-Oct-95
//-----------------------------------------------------------------------------

#include "pp.h"	      // main header file
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <alloc.h>
#include <math.h>
#include <io.h>
#include <mem.h>

extern unsigned _stklen=15000u;

PPI ppi;
PPF ppf;
PPD ppd;
PPG ppg;


/****************************** GroupWindow ********************************/

GroupWindow::GroupWindow(TopMenu *mt,BOOL sup) : grpname(NULL), tpos(0),
      sel(NULL), gsize(NULL), supply(sup), gfirst(NULL), gno(NULL),
      DataWindow(mt,Rect(1,2,3,4),0,(sup?" Предложение ":" Спрос "),
      F_DOUBLE,TRUE,TRUE){
   Display d;
   long n;
   int ml;
   if (!ReadData(n,ml)) { // error opening file!
      beep();
      MessageBox mb(" Ой ","Ошибка чтения файла данных pulse.ppg",
         "Файл не найден или поврежден...");
   } else {
      // rect, number of elements, title, frame, shadow, closebutton, parent
      if ((lno=n)==0) {
         MessageBox mb(" Объявлений нет ",
            "Нет ни одного объявления в этом разделе",
            "Спрос умер - да здравствует предложение!");
            if (grpname) {
               delete grpname;
               grpname = NULL;
            }
            return;
      }
      resize(Rect(35-ml/2,2,44+ml/2,d.yMax()-3));
      mtop->SetChecked(TRUE,supply?cmSupply:cmDemand);
      mtop->SetDisabled(FALSE,cmSelect);
      mtop->SetDisabled(FALSE,cmSelectAll);
      mtop->SetDisabled(FALSE,cmDeselectAll);
      mtop->SetDisabled(FALSE,cmSelReverse);
      draw();
      statusbar.say(
         "Выберите интересующие Вас группы товаров и нажмите ─┘");
   }
}

GroupWindow::~GroupWindow() {
   if (grpname) { // otherwise we weren't working
      for (int l=0; l<lno; l++)
         if (grpname[l])
            delete[] grpname[l];
      delete[] grpname;
      if (sel)
         delete[] sel;
      if (gsize)
         delete[] gsize;
      if (gfirst)
         delete[] gfirst;
      if (gno)
         delete[] gno;
      mtop->SetChecked(FALSE,supply?cmSupply:cmDemand);
      mtop->SetDisabled(TRUE,cmSelect);
      mtop->SetDisabled(TRUE,cmSelectAll);
      mtop->SetDisabled(TRUE,cmDeselectAll);
      mtop->SetDisabled(TRUE,cmSelReverse);
      Cursor c;
      c.hide();
   }
}

void GroupWindow::draw() {
   DataWindow::draw();
   Cursor c;
   c.moveto(cx1()+1+tpos,cy1()+(int)(lcur-lfirst));
}

BOOL GroupWindow::ReadData(long &n, int &ml) {
// FALSE if failed, long - number of groups,
// int - max length of group string
   Setup setup;
   if (!ppg.open(setup.datapath))
      return FALSE;
   n=ppg.Ngroups();
   grpname = new char*[(int)n];       // allocating space for all groups
   sel=new BOOL[(int)n];
   gsize=new long[(int)n];
   gfirst=new long[(int)n];
   gno=new int[(int)n];
   n=0;                          // but reading only supply or demand, not both
   ml=0;
   for (long l=0; l<ppg.Ngroups(); l++) {
      if (!ppg.read()) {
         ppg.close();
         return FALSE;
      }
      if (ppg.issupply==supply && ppg.gsize>0) { // this is our group
         sel[(int)n]=FALSE;
         gsize[(int)n]=ppg.gsize;
         gfirst[(int)n]=ppg.gfirst;
         gno[(int)n]=(int)l;
         grpname[(int)n++]=ppg.gname;
         int slen=strlen(ppg.gname);
         if (slen>ml)
            ml=slen;
         ppg.gname=NULL;   // not to be deleted!
      }
   }
   ppg.close();
   return TRUE;
}

BOOL GroupWindow::selected(long n) {      // is n-th line selected?
   return (n<lno && n>=0)?sel[(int)n]:FALSE;
}

void GroupWindow::select(long n, BOOL b) {    // select or deselect n-th line
   if (n<lno && n>=0) {
      if (sel[(int)n]!=b) {
         sel[(int)n]=b;
         nsel+=b?1:-1;
      }
      if (visible(n))
         repaint(1);
   }
}

char* GroupWindow::getline(long n) {      // return n-th line
   return (n<lno && n>=0 && grpname
      && grpname[(int)n])?grpname[(int)n]:"";
}

int cmps(char *s1, char *s2) { //compare two strings until first one ends
   for (char c=0; s1[c]; c++)
      if (s1[c]<s2[c])
         return -1;
      else if (s1[c]>s2[c])
         return 1;
   return 0;
}

BOOL GroupWindow::ChewEvent(Event *e) {     // should handle <Enter> key
   if (e->type==evMouseDblClick) {
      MouseEvent *m=(MouseEvent*)e;
      if (m->x>=cx1() && m->x<=cx2() && m->y<=cy2() && m->y>=cy1()) {
         if (!nsel) // if nothing is selected => select current
            sel[(int)lcur]=TRUE;
         EventQueue eq;
         for (int l=0; l<lno; l++)
            if (grpname[l] && (nsel || l!=lcur)) {
               delete[] grpname[l];
               grpname[l]=NULL; // to prevent them from being deleted twice
            }
         eq.setcommand(caOpenBaseWin);
         return TRUE;
      }
   }

   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      if (k->scancode==S_RET) {
         if (!nsel) // if nothing is selected => select current
            sel[(int)lcur]=TRUE;
         EventQueue eq;
         for (int l=0; l<lno; l++)
            if (grpname[l] && (nsel || l!=lcur)) {
               delete[] grpname[l];
               grpname[l]=NULL; // to prevent them from being deleted twice
            }
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
      return TRUE;
	} else
      return FALSE;
}

BOOL GroupWindow::ChewMenuCommand(char c) { // return TRUE if command chown
   return DataWindow::ChewMenuCommand(c);
}

/****************************** BaseWindow ********************************/

BaseWindow::BaseWindow(TopMenu *mt,FirmWindow *fw, FilterWindow *filt) :
      is_valid(FALSE), filter(filt), applyfilter(FALSE), gfound(-1),
      supply(TRUE), data(NULL), iready(0), icur(0),
		srtorder(srtUnknown), rlast(NULL), breverse(FALSE), gfirst(NULL),
      glast(NULL), findstr(NULL), savename(NULL),
      DataWindow(mt,Rect(1,2,3,4),0," Предложение ",F_DOUBLE,FALSE,TRUE) {
   Display d;
   Cursor c;
   c.hide();
   Statusbar statusbar;
   try {
      statusbar.working(TRUE);
      long n=OpenFiles(); // n - maximum number of records
      if (n<=0) { // error opening files!
			throw 15;
		}
      Setup setup;
      char *s=tempname;
      strcpy(s,setup.temppath);
      fshort=setup.fshort;
      frewrite=setup.frewrite;
   /* create a unique file and close it immediately.  we only need a name */
      close(creattemp(s, 0));
      unlink(s); // in case it won't be needed
      data=new VArrayL(n*2+n/16+2,s,2048, // NOW ALWAYS 2048!!!
         (uint)((setup.freemem-35000l)/2048/4));
      pidx=n;
      pbool=n*2;
      long delta=n/32+1;
      psel=n*2+delta;

      long nfsup=0, nfdem=0;  // number of supply and demand records found.

      for (long j=pbool; j<psel; j++)
         (*data)[j]=0;
      {
         if (!ppd.Bsize()) {
            if (ppd.setbuf(20000))
               data->SetBMax(data->BMax()-10);
         }
			void *buf=NULL;
         ControlWindow cw(Rect(5,d.yMax()/2-2,75,d.yMax()/2+2),
            " Идет поиск фирм в базе данных ",F_DOUBLE,TRUE,FALSE);
         PercentBar *pb=new PercentBar(2,1,65,0,n-1);
         cw.AddControl(pb);
         cw.Save(buf);
         statusbar.say("Идет поиск... Нажмите <ESC> для прерывания.");
         cw.draw();

         EventQueue eq;
         for (j=0; j<n; j++) { // searching supply records
            if (!ppd.read(ppi(j,TRUE),FALSE)) {
               statusbar.working(FALSE);
               cw.Restore(buf);
               if (ppd.Bsize())
                  data->SetBMax(data->BMax()+10);
               ppd.delbuf();
               throw 2;
            }
            if (fw->selected(ppd.drec.fcode)) {
               nfsup++;
               setbool(j,TRUE);
            }
            Event *e=NULL;
            if (j%100==80) {
					do {
                  if (e)
                     delete e;
                  e=eq.get();
                  pb->setpos(j);
                  if (e && e->type==evKeyPress) {
                     KeyEvent *k=(KeyEvent*)e;
                     if (k->scancode==S_ESC) {
                        cw.Restore(buf);
                        if (ppd.Bsize())
                           data->SetBMax(data->BMax()+10);
                        ppd.delbuf();
                        if (nfsup>0) {
                           lno=nfsup;
                           goto cont1;
                        }
                        statusbar.working(FALSE);
                        delete fw;
                        eq.setcommand(caCloseDataWin);
                        savesb=statusbar.get();
                        return;
                     }
                  }
               } while (e);
               if (e)
						delete e;
            }
         }
         cw.Restore(buf);
         if (ppd.Bsize())
            data->SetBMax(data->BMax()+10);
         ppd.delbuf();
      }
      if (nfsup>0)
         lno=nfsup;
      else {   // searching demand records....
         supply = FALSE;
         title = " Спрос ";
         for (j=0; j<n; j++) { // searching demand records
            if (!ppd.read(ppi(j,TRUE),TRUE)) {
               statusbar.working(FALSE);
               throw 3;
            }
            if (fw->selected(ppd.drec.fcode)) {
               nfdem++;
               setbool(j,TRUE);
            }
         }
         if (nfdem>0)
            lno=nfdem;
			else {
            beep();
            MessageBox mb(" Ошибка в базе ",
               "Ни одного объявления выбранных Вами фирм не найдено",
               "Ошибка исходной базы данных...");
            EventQueue eq;
            delete fw;
            eq.setcommand(caCloseDataWin);
            savesb=statusbar.get();
            return;
         }
      }
   } catch (int i) {
      statusbar.working(FALSE);
      beep();
      MessageBox mb(" Ой ","Ошибка чтения файлов данных",
         "Файлы не найдены или повреждены...");
      if (i>1) {
         ppi.close();
         ppd.close();
         ppf.close();
         ppg.close();
      }
      EventQueue eq;
      delete fw;
		eq.setcommand(caCloseDataWin);
      savesb=statusbar.get();
      return;
   } catch (VArrayErr ve) {
      statusbar.working(FALSE);
      ppi.close();
      ppd.close();
      ppf.close();
      delete fw;
      if (ve.code()==vaFCreateFault || ve.code()==vaFWriteFault) {
         beep();
         MessageBox mb(" Ой ","Ошибка записи во временный файл.",
            "Проверьте наличие места, правильность настройки каталогов...");
      } else
         throw ve; // throw the error further
   }
cont1:   // label to go to when ESC is pressed, but something was already found
   delete fw; // now old group window can be deleted
   savename=new char[70];
   strcpy(savename,"goods.txt");
   resize(Rect(0,1,d.xMax(),d.yMax()-1));
   mtop->SetChecked(TRUE,supply?cmSupply:cmDemand);
   mtop->SetDisabled(FALSE,cmSelect);
   mtop->SetDisabled(FALSE,cmSelectAll);
   mtop->SetDisabled(FALSE,cmDeselectAll);
	mtop->SetDisabled(FALSE,cmSelReverse);
   mtop->SetDisabled(FALSE,cmFind);
   mtop->SetDisabled(FALSE,cmFindNext);
   mtop->SetDisabled(FALSE,cmSaveCur);
//   mtop->SetDisabled(FALSE,cmDelSel);
   roubleprice=mtop->IsChecked(cmRoublePrice);
   filtering=applyfilter=mtop->IsChecked(cmApplyFilter);
   showbrief=mtop->IsChecked(cmShowBrief);
   sbtoggled=!showbrief;
   srtorder=mtop->IsChecked(cmNoSort)?srtNatural:
      mtop->IsChecked(cmSortPrices)?srtAscendPrices:
      mtop->IsChecked(cmSortPricesDesc)?srtDescendPrices:srtUnknown;
   draw();
   savesb=statusbar.get();
   statusbar.say(
      "Нажмите ─┘ для переключения режимов полного/краткого вывода");
}


// constructor when going from group window
BaseWindow::BaseWindow(TopMenu *mt,GroupWindow *gw, FilterWindow *filt) :
		is_valid(FALSE), filter(filt), applyfilter(FALSE), gfound(-1),
		supply(gw->issupply()), data(NULL), iready(0), icur(0),
		srtorder(srtUnknown), rlast(NULL), breverse(FALSE), gfirst(NULL),
		glast(NULL), findstr(NULL), savename(NULL),
		DataWindow(mt,Rect(1,2,3,4),0,(gw->issupply()?" Предложение ":" Спрос "),
		F_DOUBLE,FALSE,TRUE) {
	Display d;
	Cursor c;
	c.hide();
	Statusbar statusbar;
	try {
		statusbar.working(TRUE);
		long n=OpenFiles();
		if (n<=0) { // error opening files!
			throw 16;
		}
		if (gw->CurName()) {
			mytitle[0]=' ';
			mytitle[1]=0;
			char *s=gw->CurName();
			if (strlen(s)>=48)
				s[48]=0;
			strcat(mytitle,s);
			strcat(mytitle," ");
			title=mytitle;
			gfound=-2;  // only one group
		}
		Setup setup;
		fshort=setup.fshort;
		frewrite=setup.frewrite;
		char *s=tempname;
		strcpy(s,setup.temppath);
		close(creattemp(s, 0)); // get a temp file name
		unlink(s);
		data=new VArrayL(n*2+n/16+2,s,2048,
			(uint)((setup.freemem-35000l)/2048/4));
		pidx=n;
		pbool=n*2;
		psel=n*2+n/32+1;
		if (!ppi.iselect(supply?iSupply:iDemand)) {
			throw 2;
		}
		long dcur=0;
		if (supply) {
			gfirst=new long[ppi.npaym];
			glast=new long[ppi.npaym];
			for (int i=0; i<ppi.npaym; i++) { // for each payment type
				ppi.iselect(iPaym+i*2);
				gfirst[i]=ppi.RecNo();
				glast[i]=-1;
				for (int gr=0; gr<gw->Lno(); gr++)
					if (gw->selected(gr)) { // selected group found
						long gf,gl;
						if (!ppi.readg(gw->Gno(gr),gf,gl)) {
							throw 17;
						}
						if (gf<gfirst[i] || gfirst[i]==-1)
							if (gf!=-1)
								gfirst[i]=gf;
						if (gl>glast[i])
							glast[i]=gl;
					}
			}
		} // if supply
		for (long l=0; l<gw->Lno(); l++) {
			BOOL b=gw->selected(l);
			for (long j=gw->Gfirst(l); j<gw->Gfirst(l)+gw->Gsize(l); j++) {
				if (j>=n) {
//					MessageBox("aa,","bb");
					throw 18;
				}
				if (j%32==0 && j+31<gw->Gfirst(l)+gw->Gsize(l)) {
					(*data)[pbool+j/32]=b?0xffffffffl:0;
					j+=31;
				} else
					setbool(j,b);
			}
			dcur+=b?gw->Gsize(l):0;
		}
		lno=dcur; // this many elements we have...
	} catch (int i) {
		statusbar.working(FALSE);
		beep();
		MessageBox mb(" Ой ","Ошибка чтения файлов данных",
			"Файлы не найдены или повреждены...");
		EventQueue eq;
		eq.setcommand(caCloseDataWin);
		delete gw;
		return;
	} catch (VArrayErr ve) {
		statusbar.working(FALSE);
		ppi.close();
		ppd.close();
		ppf.close();
		ppg.close();
		if (ve.code()==vaFCreateFault || ve.code()==vaFWriteFault) {
			beep();
			MessageBox mb(" Ой ","Ошибка записи во временный файл.",
				"Проверьте наличие места, правильность настройки каталогов...");
		} else
			throw ve; // throw the error further
	}
	delete gw; // now old group window can be deleted
	savename=new char[70];
	strcpy(savename,"goods.txt");
	resize(Rect(0,1,d.xMax(),d.yMax()-1));
	mtop->SetChecked(TRUE,supply?cmSupply:cmDemand);
	mtop->SetDisabled(FALSE,cmSelect);
	mtop->SetDisabled(FALSE,cmSelectAll);
	mtop->SetDisabled(FALSE,cmDeselectAll);
	mtop->SetDisabled(FALSE,cmSelReverse);
	mtop->SetDisabled(FALSE,cmFind);
	mtop->SetDisabled(FALSE,cmFindNext);
	mtop->SetDisabled(FALSE,cmSaveCur);
//   mtop->SetDisabled(TRUE,cmDelSel);
	roubleprice=mtop->IsChecked(cmRoublePrice);
	filtering=applyfilter=mtop->IsChecked(cmApplyFilter);
	showbrief=mtop->IsChecked(cmShowBrief);
	sbtoggled=!showbrief;
	srtorder=mtop->IsChecked(cmNoSort)?srtNatural:
		mtop->IsChecked(cmSortPrices)?srtAscendPrices:
		mtop->IsChecked(cmSortPricesDesc)?srtDescendPrices:srtUnknown;
	draw();
	savesb=statusbar.get();
	statusbar.say(
		"Нажмите ─┘ для переключения режимов полного/краткого вывода");

}

long BaseWindow::OpenFiles() { // returns max number of data records, 0 => fail
	Setup setup;

	if (!ppi.open(setup.datapath)) {
		return 0;
	}
	if (!ppd.open(setup.datapath)) {
		ppi.close();
		return 0;
	}
	if (!ppf.open(setup.datapath)) {
		ppi.close();
		ppd.close();
		return 0;
	}
	if (!ppg.open(setup.datapath)) {
		ppi.close();
		ppd.close();
		ppf.close();
		return 0;
	}
	try {
		if (!ppi.iselect(iSupply))
			throw 19;
		long n=ppi.RecNo();
		if (!ppi.iselect(iDemand)) {
			throw 2;
		}
		if (n<ppi.RecNo())
			n=ppi.RecNo();
		return n;
	} catch (int) {
		ppi.close();
		ppd.close();
		ppf.close();
		ppg.close();
		return 0;
	}
}

inline BOOL getbit(ulong u, int ofs) { // get bit on a given ofs
   return (BOOL)((u>>ofs)&1);
}

inline void setbit(ulong &u, int ofs, BOOL b) { // set bit on ofs to b
   ulong us=((ulong)1<<ofs);
   if (b)
      u|=us;   // set bit
   else
      u&=~us;  // clear bit
}

inline void BaseWindow::setbool(long l,BOOL b) {
   setbit((*data)[pbool+l/32],(int)(l%32),b);
}

inline void BaseWindow::setsel(long l,BOOL b) {
   setbit((*data)[psel+l/32],(int)(l%32),b);
}

inline BOOL BaseWindow::getbool(long l) {
   return getbit((*data)(pbool+l/32),(int)(l%32));
}

inline BOOL BaseWindow::getsel(long l) {
   return getbit((*data)(psel+l/32),(int)(l%32));
}

BOOL BaseWindow::ReadData(long n) { // read n index elements, FALSE if failed
   Rates rates;
   Statusbar st;
   EventQueue eq;
   Mouse m;
   BOOL rsz=FALSE;   // need resizing!
   unsigned mh=0;
   if (n>50 || (srtorder!=srtNatural && !rlast))
      while (!m.shown()) {
         mh++;
         m.show();
      }
   if (srtorder==srtUnknown)
		throw 10; // this must not happen again ever!
	if (srtorder!=srtNatural && !rlast && !breverse) {
		if (/*srtorder==srtAscendPrices && */!ppi.Bsize2()) {
			if (ppi.setbuf2(14000))
				data->SetBMax(data->BMax()-8);
		}
		// initializing positions for all
		pcdone=0;
		lastpc=-1;  // last percent shown
		rlast = new long[ppi.npaym];        // payment indexes.
		rprice = new float[ppi.npaym];
		for (int i=0; i<ppi.npaym; i++) {
			rlast[i]=0;
			if (supply && gfirst && srtorder==srtAscendPrices) {
				rlast[i]=gfirst[i];
				pcdone+=gfirst[i];
			}
			ppi.iselect(iPaym+i*2+(supply?0:1));
			if (supply && glast && srtorder==srtDescendPrices) {
				rlast[i]=ppi.RecNo()-glast[i]-1;
				pcdone+=rlast[i];
			}
			int counter=0;
			while (rlast[i]!=-1 && rlast[i]<ppi.RecNo() && !getbool(ppi[
					srtorder==srtAscendPrices?rlast[i]:ppi.RecNo()-rlast[i]-1])) {
				rlast[i]++; pcdone++;
				if (++counter==50 && pcdone%50==0 && (float)pcdone*100/pidx!=lastpc) {
					counter=0;
					lastpc=(float)pcdone*100/pidx;
					char ss[10];
					sprintf(ss," %3u%% ",lastpc);
					VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
					eq.CheckEvents();
				}
			}
			if (rlast[i]>=ppi.RecNo())
				rlast[i]=-1;
			else if (ppd.read(ppi(ppi[
				srtorder==srtAscendPrices?rlast[i]:ppi.RecNo()-rlast[i]-1],
				supply),FALSE))
				rprice[i]=ppd.drec.price*rates[i];
			else
				return FALSE;
		}
	}
	static int counter=0;
	do {
		if (iready>=lno) { // done working
			st.working(FALSE);
			if (ppi.Bsize1())
				data->SetBMax(data->BMax()+8);
			if (ppi.Bsize2())
				data->SetBMax(data->BMax()+8);
			ppi.delbuf1();
			ppi.delbuf2();
			if (!filtering) {
				VSetColor(x1()+5,y1(),10,attr[attrListWin]);
				VSetChar(x1()+5,y1(),10,205);
			}
			iready=lno;
			if (rsz)
				resize(rect);
			return TRUE;
		}
		if (breverse) {  // just reversing the sort order
			long l1=(*data)(iready);
			(*data)[iready]=(*data)(lno-iready-1);
			(*data)[lno-iready-1]=l1;
			l1=(*data)(iready+pidx);
			(*data)[iready+pidx]=(*data)(lno-iready-1+pidx);
			(*data)[lno-iready-1+pidx]=l1;
			if (++iready>=lno/2) {
				iready=lno;    // done!
				breverse=FALSE;
			}
			if (++counter==50 && (float)iready*200/lno!=lastpc) {
            counter=0;
            lastpc=(float)iready*200/lno;
            char ss[10];
            sprintf(ss," %3u%% ",lastpc);
            VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
         }
      } else if (srtorder==srtNatural) { // here handling natural order
         if (!ppi.Bsize1()) {
            if (ppi.setbuf1(14000))
               data->SetBMax(data->BMax()-8);
         }
         if (icur%32==0 && (*data)(pbool+icur/32)==0xffffffffl) {
            for (long l=min(icur+32,pidx); icur<l; ) {
               long pp=ppi(icur++,supply);
               n--;
               if (applyfilter && !(*filter)[pp]) {
                  setbool(icur-1,FALSE);
                  lno--;
                  rsz=TRUE;
               } else
                  (*data)[iready++]=pp;  // <-------
            }
            n++; // need to increment once since it'll be decremented later
         } else {
            if (icur%32==0)
               while ((*data)(pbool+icur/32)==0)
                  if ((icur+=32)>=pidx)
                     return FALSE;
            while (!getbool(icur)) {
               if (++icur>=pidx)
                  return FALSE;
            }
            if (!filtering && ++counter==50 && (float)icur*100/pidx!=lastpc) {
               counter=0;
               lastpc=(float)icur*100/pidx;
               char ss[10];
               sprintf(ss," %3u%% ",lastpc);
               VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
               eq.CheckEvents();
            }
            long pp=ppi(icur++,supply);
            if (applyfilter && !(*filter)[pp]) {
               setbool(icur-1,FALSE);
               lno--;
               rsz=TRUE;
            } else
               (*data)[iready++]=pp;  // <-------
			}
		} else { // non-natural order
			float extr=(srtorder==srtAscendPrices)?1e20:-1e20;
			int pos=-1;  // extremum position
			long ppipos=0;
			// let's find our extremum
			int i0=-1;
			for (int i=0; i<ppi.npaym; i++) {
				if (rlast[i]<0)
					continue;
				if (i0!=i) {
					ppi.iselect(iPaym+i*2+(supply?0:1));
					i0=i;
				}
				if (srtorder==srtAscendPrices) {
					if (rlast[i]<ppi.RecNo() &&
						rprice[i]<extr) {
						pos=i;
						extr=rprice[i];
					}
				} else { // descending sort
					if (rlast[i]<ppi.RecNo() &&
						rprice[i]>extr) {
						pos=i;
						extr=rprice[i];
					}
				}
			} // extremum found.
			if (pos==-1) {
//				MessageBox("here","here");
				throw 20;
			}
			ppi.iselect(iPaym+pos*2+(supply?0:1));
			if (srtorder==srtAscendPrices)
				ppipos=ppi[rlast[pos]];
			else
				ppipos=ppi[ppi.RecNo()-rlast[pos]-1];
			rlast[pos]++;
			pcdone++;
			long pp=ppi(ppipos,supply);
			if (applyfilter && !(*filter)[pp]) {
				setbool(ppipos,FALSE);
				lno--;
				rsz=TRUE;   // need resizing
/*            n++; */
			} else
				(*data)[iready++]=pp; // <-------------
			// it's stored!  Now finding next correct index element
			ppi.iselect(iPaym+pos*2+(supply?0:1));
			while (rlast[pos]<ppi.RecNo() && !getbool(ppi[
            srtorder==srtAscendPrices?rlast[pos]:ppi.RecNo()-rlast[pos]-1])) {
            rlast[pos]++;
            pcdone++;
            if (++counter==50) {
               counter=0;
               lastpc=(float)pcdone*100/pidx;
               char ss[10];
               sprintf(ss," %3u%% ",lastpc);
               VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
               eq.CheckEvents();
            }
         }
         if (++counter==50 && (float)pcdone*100/pidx!=lastpc) {
            counter=0;
            lastpc=(float)pcdone*100/pidx;
            char ss[10];
            sprintf(ss," %3u%% ",lastpc);
            VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
         }
         if (rlast[pos]>=ppi.RecNo() ||
            (srtorder==srtAscendPrices && glast && rlast[pos]>glast[pos]) ||
            (srtorder==srtDescendPrices && gfirst &&
               ppi.RecNo()-rlast[pos]-1<gfirst[pos]))
            rlast[pos]=-1;
         else
            if (ppd.read(ppi(ppi[
               srtorder==srtAscendPrices?rlast[pos]:ppi.RecNo()-rlast[pos]-1],
               supply),FALSE))
               rprice[pos]=ppd.drec.price*rates[pos];
            else
               return FALSE;
      }
   } while (--n>0);
   if (iready==lno) {
      st.working(FALSE);
      if (ppi.Bsize1())
         data->SetBMax(data->BMax()+8);
      if (ppi.Bsize2())
         data->SetBMax(data->BMax()+8);
      ppi.delbuf1();
      ppi.delbuf2();
      if (!filtering) {
         VSetColor(x1()+5,y1(),10,attr[attrListWin]);
         VSetChar(x1()+5,y1(),10,205);
      }
   }
   while (mh--)
      m.hide();
   if (rsz)
      resize(rect);
   return TRUE;
}

BaseWindow::~BaseWindow() {
   if (data)
      delete data;
   if (rlast) {
      delete[] rlast;
      delete[] rprice;
   }
   if (findstr)
      delete[] findstr;
   if (savename)
      delete[] savename;
   if (gfirst)
      delete[] gfirst;
   if (glast)
      delete[] glast;
   ppi.close();
   ppd.close();
   ppf.close();
   ppg.close();
   Statusbar s;
   s.working(FALSE);
   mtop->SetChecked(FALSE,supply?cmSupply:cmDemand);
   mtop->SetDisabled(TRUE,cmSelect);
   mtop->SetDisabled(TRUE,cmSelectAll);
   mtop->SetDisabled(TRUE,cmDeselectAll);
   mtop->SetDisabled(TRUE,cmSelReverse);
   mtop->SetDisabled(TRUE,cmFind);
   mtop->SetDisabled(TRUE,cmFindNext);
   mtop->SetDisabled(TRUE,cmSaveCur);
   mtop->SetDisabled(TRUE,cmDelSel);
}

BOOL BaseWindow::selected(long n) {      // is n-th line selected?
   if (n>=lno)
      return FALSE;
   if (n>=iready)
      if (!ReadData(n-iready+1))
         throw -1;
   return ((*data)(n) & 0x80000000l)!=0;
}

void BaseWindow::select(long n, BOOL b) {    // select or deselect n-th line
   if (n>=lno)
      return;
   if (selected(n)!=b)
      nsel+=b?1:-1;
   if (b)
      (*data)[n] |= 0x80000000l;
   else
      (*data)[n] &= 0x7fffffffl;
   mtop->SetDisabled(nsel==0,cmDelSel);
   if (visible(n))
      repaint(1);
}

BOOL BaseWindow::ChewEvent(Event *e) {     // should handle <Enter> key
   try {
      if (e->type==evKeyPress) {
         KeyEvent *k=(KeyEvent*)e;
         switch (k->scancode) {
            case S_RET:
               EventQueue eq;
               eq.setcommand(cmShowBrief);
               return TRUE;
         }
      } else if (e->type==evMouseDblClick) {
         MouseEvent *m=(MouseEvent*)e;
         if (m->x>=cx1() && m->x<=cx2() && m->y>=cy1() && m->y<=cy2())
            return ChewMenuCommand(cmShowBrief);
      }
      if (DataWindow::ChewEvent(e)) {
         if (ncode==ncCloseButton) { // close the window
            EventQueue eq;
            eq.setcommand(caCloseDataWin);
         }
         return TRUE;
      } else
         return FALSE;
   } catch (VArrayErr va) {
      if (va.code()==vaFWriteFault || va.code()==vaFCreateFault) {
         beep();
         MessageBox mb(" Ой ","Ошибка записи во временный файл.",
            "Проверьте наличие места, правильность настройки каталогов...");
         EventQueue eq;
         eq.setcommand(caCloseDataWin);
         return TRUE;
      } else
         throw va; // throw the error further
   }
}

BOOL BaseWindow::ChewMenuCommand(char c) { // return TRUE if command chown
   char ss[10];
   int lastpc;
   Statusbar sb;
   try {
      if (c==cmFindNext && findstr==NULL)
         c=cmFind;
      long l;
      switch (c) {
         case cmRoublePrice:
            mtop->SetChecked(roubleprice=!roubleprice,cmRoublePrice);
            repaint(1);
            return TRUE;
         case cmShowBrief:
            mtop->SetChecked(showbrief=!showbrief,cmShowBrief);
            sbtoggled=TRUE;
            repaint(255);
            return TRUE;
         case cmApplyFilter:
            mtop->SetChecked(applyfilter=!applyfilter,cmApplyFilter);
            if (applyfilter) {
               if (rlast) {
                  delete[] rlast;
                  rlast=NULL;
                  delete[] rprice;
               }
               filtering=TRUE;
               lfirst=lcur=iready=icur=0;
               repaint(1);
            }
            return TRUE;
         case cmSelectAll:
            sb.working(TRUE);
            for (l=0; l<lno; l++) {
               (*data)[l] |= 0x80000000l;
               if (l%100==99) {
                  lastpc=(float)l*100/lno;
                  sprintf(ss," %3u%% ",lastpc);
                  VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
               }
            }
            nsel=lno;
            mtop->SetDisabled(FALSE,cmDelSel);
            repaint(1);
            VSetColor(x1()+5,y1(),10,attr[attrListWin]);
            VSetChar(x1()+5,y1(),10,205);
            sb.working(FALSE);
            return TRUE;
         case cmDeselectAll:
            sb.working(TRUE);
            for (l=0; l<lno; l++) {
               (*data)[l] &= 0x7fffffffl;
               if (l%100==99) {
                  lastpc=(float)l*100/lno;
                  sprintf(ss," %3u%% ",lastpc);
                  VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
               }
            }
            nsel=0;
            mtop->SetDisabled(TRUE,cmDelSel);
            repaint(1);
            VSetColor(x1()+5,y1(),10,attr[attrListWin]);
            VSetChar(x1()+5,y1(),10,205);
            sb.working(FALSE);
            return TRUE;
         case cmSelReverse:
            sb.working(TRUE);
            for (l=0; l<lno; l++) {
               if (!selected(l))
                  (*data)[l] |= 0x80000000l;
               else
                  (*data)[l] &= 0x7fffffffl;
               if (l%100==99) {
                  lastpc=(float)l*100/lno;
                  sprintf(ss," %3u%% ",lastpc);
                  VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
               }
            }
            nsel=lno-nsel;
            mtop->SetDisabled(nsel==0,cmDelSel);
            repaint(1);
            VSetColor(x1()+5,y1(),10,attr[attrListWin]);
            VSetChar(x1()+5,y1(),10,205);
            sb.working(FALSE);
            return TRUE;
         case cmDelSel: { // remove selection from current record set
				if (breverse || nsel==lno) {
					beep();
					return TRUE;
				}
				sb.working(TRUE);
				long ic=0;
				for (l=0; l<lno; ) {
					while (!(*data)(pbool+ic))
						ic++; // finding the place...
					for (int i=0; i<32; i++) {
						if (getbool(ic*32+i)) {
							if (selected(l))
								setbool(ic*32+i,FALSE);
							l++;
							if (l%100==99) {
								lastpc=(float)l*100/lno;
								sprintf(ss," %3u%% ",lastpc);
								VPutsColor(x1()+5,y1(),ss,attr[attrListWinSel]);
							}
						}
					}
					ic++;
				}
				lno-=nsel;
				nsel=0;
				mtop->SetDisabled(TRUE,cmDelSel);
				icur=iready=0;
				if (rlast) {
					delete rlast;
					rlast=NULL;
				}
				sb.working(FALSE);
				lcur=lfirst=0;

/*				rect.y2-=7;
				sbtoggled=TRUE;   // to reinitialize the scrollbar in draw() */
				resize(Rect(rect.x1,rect.y1,rect.x2,rect.y2));

				moveto(lcur);
				repaint(255);
				return TRUE;
			}
         case cmNoSort:
            if (srtorder!=srtNatural) {
               sb.working(TRUE);
               mtop->SetChecked(FALSE,cmSortPrices);
               mtop->SetChecked(FALSE,cmSortPricesDesc);
               mtop->SetChecked(TRUE,cmNoSort);
               if (rlast) {
                  delete[] rlast;
                  rlast=NULL;
                  delete[] rprice;
               }
               srtorder=srtNatural;
               iready=0; icur=0;
               moveto(0);
               repaint(1);
            }
            return TRUE;
         case cmSortPrices:
            if (breverse || iready<lno) {
               beep();
               return TRUE; // if we're doing reversing now, must finish first
            }
            if (srtorder!=srtAscendPrices) {
               sb.working(TRUE);
               mtop->SetChecked(FALSE,cmNoSort);
               mtop->SetChecked(FALSE,cmSortPricesDesc);
               mtop->SetChecked(TRUE,cmSortPrices);
               breverse=(srtorder==srtDescendPrices);
               if (!breverse) {
                  nsel=0;
                  mtop->SetDisabled(TRUE,cmDelSel);
               }
               srtorder=srtAscendPrices;
               if (rlast) {
                  delete[] rlast;
                  rlast=NULL;
                  delete[] rprice;
               }
               iready=0;
               lcur=lfirst=0;
               repaint(1);
            }
            return TRUE;
         case cmSortPricesDesc:
            if (breverse || iready<lno) {
               beep();
               return TRUE; // if we're doing reversing now, must finish first
            }
            if (srtorder!=srtDescendPrices) {
               sb.working(TRUE);
               mtop->SetChecked(FALSE,cmNoSort);
               mtop->SetChecked(FALSE,cmSortPrices);
               mtop->SetChecked(TRUE,cmSortPricesDesc);
               breverse=(srtorder==srtAscendPrices);
               if (!breverse) {
                  nsel=0;
                  mtop->SetDisabled(TRUE,cmDelSel);
               }
               srtorder=srtDescendPrices;
               if (rlast) {
                  delete[] rlast;
                  rlast=NULL;
                  delete[] rprice;
               }
               iready=0;
               lfirst=lcur=0;
               repaint(1);
            }
            return TRUE;
         case cmFind: {
            char *s000;
            s000=sb.get();
            void *buf;
            char nc;
            buf=NULL;
            Display d;
            {
               ControlWindow cw(Rect(5,d.yMax()/2-3,75,d.yMax()/2+3),
                  " Введите строку для поиска: ",F_DOUBLE,TRUE,TRUE);
               InputField *ib=new InputField(1,1,67,findstr,
                     (InputField::InpType)(InputField::it_any+InputField::it_caps),
                     "Поиск подстроки в базе данных");
               // name, sel.char.pos, x,y , ncode, hotkey, def, saying, parent
               Button *b=new Button(" Искать ",2,30,3,ncOkButton,0,TRUE);
               cw.AddControl(b);
               cw.AddControl(ib);
               cw.Save(buf);
               cw.draw();
               nc=cw.GoModal();
               cw.Restore(buf);
            }
            sb.say(s000);
            if (nc!=ncOkButton)
               return TRUE;   // do not find.
            }
         case cmFindNext:
            {  // finding the substring findstr
               char *s000=sb.get();
               sb.say("Идет поиск... Нажмите <ESC> для прерывания.");
               void *buf=NULL;
               Display d;
               ControlWindow cw(Rect(5,d.yMax()/2-2,75,d.yMax()/2+2),
                  " Идет поиск... ",F_DOUBLE,TRUE,FALSE);
               PercentBar *pb=new PercentBar(2,1,65,0,lno-1);
               cw.AddControl(pb);
               cw.Save(buf);
               cw.draw();
               long ll=lcur;
               EventQueue eq;
               sb.working(TRUE);
               int counter=0;
               if (srtorder==srtNatural && !ppd.Bsize()) {
//                  data->flush();
                  if (ppd.setbuf(20000))
                     data->SetBMax(data->BMax()-10);
               }
               while (++ll<lno) {
                  if (ll>=iready)
                     if (!ReadData(ll-iready+1)) {
                        MessageBox(" BaseWindow::ChewMenuCommand 1 ",
                           "Ошибка чтения файлов данных",
                           "Файлы не найдены или повреждены...");
                        if (ppd.Bsize())
                           data->SetBMax(data->BMax()+10);
                        ppd.delbuf();
                        cw.Restore(buf);
                        sb.say(s000);
                        sb.working(FALSE);
                        return TRUE;
                     }
                  if (!ppd.read((*data)(ll) & 0x7fffffffl)) {
                        MessageBox(" BaseWindow::ChewMenuCommand 2 ",
                           "Ошибка чтения файлов данных",
                           "Файлы не найдены или повреждены...");
                        if (ppd.Bsize())
                           data->SetBMax(data->BMax()+10);
                        ppd.delbuf();
                        cw.Restore(buf);
                        sb.say(s000);
                        sb.working(FALSE);
                        return TRUE;
                  }
                  if (strstr(ppd.drec.ad,findstr)) { // found!
                     sb.working(FALSE);
                     cw.Restore(buf);
                     moveto(ll);
                     sb.say(s000);
                     if (ppd.Bsize())
                        data->SetBMax(data->BMax()+10);
                     ppd.delbuf();
                     return TRUE;
                  }
                  Event *e=NULL;
                  if (++counter==100) {
                     counter=0;
                     do {
                        if (e)
                           delete e;
                        e=eq.get();
                        pb->setpos(ll);
                        if (e && e->type==evKeyPress) {
                           KeyEvent *k=(KeyEvent*)e;
                           if (k->scancode==S_ESC) {
                              sb.working(FALSE);
                              cw.Restore(buf);
                              sb.say(s000);
                              if (ppd.Bsize())
                                 data->SetBMax(data->BMax()+10);
                              ppd.delbuf();
                              return TRUE;
                           }
                        }
                     } while (e);
//                     delete e;
                  }
               }
               if (ppd.Bsize())
                  data->SetBMax(data->BMax()+10);
               ppd.delbuf();
               cw.Restore(buf);
               sb.say(s000);
               // substring not found
               sb.working(FALSE);
               beep();
               MessageBox mb(" Поиск неудачен ",
               "Введенная Вами строка не найдена в выборке");
            }
            return TRUE;
			case cmSaveCur: {
				if (nsel>1000) {
					MessageBox(" Переполнение буфера ",
					" Отмечено более 1000 записей. Сохранение в файл невозможно.");
					return TRUE;
				}
				char *s000;
				s000=sb.get();
				void *buf;
				char nc;
				buf=NULL;
				Display d;
				{
					ControlWindow cw(Rect(5,d.yMax()/2-5,75,d.yMax()/2+5),
						" Имя файла для записи: ",F_DOUBLE,TRUE,TRUE);
					InputField *ib=new InputField(1,1,67,savename,
							(InputField::InpType)(InputField::it_filename),
						 "Для печати на принтере наберите PRN вместо имени файла");
					// name, sel.char.pos, x,y , ncode, hotkey, def, saying, parent
					Button *b=new Button(" Записать ",2,29,7,ncOkButton,A_ALT_P,TRUE);
					RadioButton rb[]={
						RadioButton(3,3,"Полная форма",7,3),
						RadioButton(3,4,"Краткая форма",7,4),
						RadioButton(3,5,"Список товаров + адреса фирм",7,5)};
					RadioButtonGroup *rbg=new RadioButtonGroup(rb,3,fshort,
						"Выберите форму записи информации в файл.");
					RadioButton rb2[]={
						RadioButton(30,3,"Дописать в конец файла",34,3),
						RadioButton(30,4,"Переписать существующий файл",34,4)};
					RadioButtonGroup *rbg2=new RadioButtonGroup(rb2,2,frewrite,
						"Выберите что делать если файл существует.");
					b->SetTTLRUD(ib,rbg2,rbg,rbg2,ib);
					ib->SetTTLRUD(rbg,b,NULL,NULL,NULL,rbg);
					rbg->SetTTLRUD(rbg2,ib,NULL,rbg2,ib,b);
					rbg2->SetTTLRUD(b,rbg,rbg,NULL,ib,b);
					cw.AddControl(b);
					cw.AddControl(rbg);
					cw.AddControl(rbg2);
					cw.AddControl(ib);
					cw.Save(buf);
					cw.draw();
					nc=cw.GoModal();
					if (nc) {
						Setup set;
						set.frewrite=frewrite=rbg2->selected();
						set.fshort=fshort=rbg->selected();
					}
					cw.Restore(buf);
				}
				sb.say(s000);
				if (nc!=ncOkButton)
					return TRUE;   // do not save
				sb.working(TRUE);
				{  // saving
					ulong *fise=NULL;  // uled firms list
					if (fshort==2) {
						fise = new ulong[ppf.Nfirms()/32+1];
						memset(fise, 0, (ppf.Nfirms()/32+1)*4);
					}
					char *s000=sb.get();
					sb.say("Идет запись... Нажмите <ESC> для прерывания.");
					void *buf=NULL;
					Display d;
					ControlWindow cw(Rect(5,d.yMax()/2-2,75,d.yMax()/2+2),
						" Идет запись в файл... ",F_DOUBLE,TRUE,FALSE);
					PercentBar *pb=new PercentBar(2,1,65,0,lno-1);
					cw.AddControl(pb);
					cw.Save(buf);
					cw.draw();
					long ll=-1;
					EventQueue eq;
					sb.working(TRUE);
					ofstream ff(savename,ios::out | (frewrite?0:ios::app));
					if (!ff) {
						beep();
						MessageBox mb("Ошибка создания файла",
							"Проверьте правильность ввода имени файла");
						return TRUE;
					}
					while (++ll<lno) {
/*                  if (nsel==0)
							if (ll<lcur)
								ll=lcur;
							else
                        break; */
						if (ll>iready && !breverse)
							break;   // there can't be any more
						if (selected(ll) || nsel==0)
							if (fshort) {
								ff << getline(ll) << endl;
								if (fise /*&& getfullinfo(ll)*/) {
									int fn=ppd.drec.fcode;
									setbit(fise[fn/32],fn%32,TRUE);
								}
							} else
								if (getfullinfo(ll)) {
/*                           char s[15];
									sprintf(s," %02d.%02d.%02d ",ppd.drec.dsubmit.day(),
										ppd.drec.dsubmit.month(),ppd.drec.dsubmit.yy()); */
									ff << "=================================================="
											"====================\n";

									// finding group
									while (gfound!=-2 && gfound!=ll) {
										long dd=(*data)(ll) & 0x7fffffffl;
										if (ppg.issupply==supply &&
											 ppi(ppg.gfirst,supply)<=dd &&
											 ppi(ppg.gfirst+ppg.gsize,supply)>dd) {
											 gfound=ll;  // group found;
											 break;
										}
										if (ppg.Gno()==ppg.Ngroups())
											ppg.rewind();
										ppg.read();
									}
									// end finding group

									ff << "Группа: " << (gfound==-2? title : ppg.gname) << endl;

/*                           ff << "Дата подачи объявления: " << s << endl; */
									char str[5][80];
									strcpy(str[0],"Товар: ");
									if (strlen(ppd.drec.ad)<68) {
										strcat(str[0],ppd.drec.ad);
										str[1][0]=0;
									} else {
										strcpy(str[1],"       ");
										for (char *c=ppd.drec.ad+68; *c!=' ' &&
												c>ppd.drec.ad; c--)
											if (c==ppd.drec.ad)
												c+=68;
											*c++=0;
										strcat(str[0],ppd.drec.ad);
										c[69]=0;
										strcat(str[1],c);
									}
									if (!ppf.fullname || strlen(ppf.fullname)<2)
										sprintf(str[2],"Фирма: %s",ppf.firm);
									else {
										int ii=77-9-strlen(ppf.firm);
										if (strlen(ppf.fullname)>ii)
											ppf.fullname[ii]=0;
										sprintf(str[2],"Фирма: %s, %-s",ppf.firm,ppf.fullname);
									}
									sprintf(str[3],"Адрес: %s",ppf.address);
									sprintf(str[4],"Телефоны: (%03lu) ",ppf.area);
									for (int i=0; i<4; i++)
										if (ppf.phone[i]) {
											if (i==3)
												strcat(str[4],", Факс: ");
											else if (i)
												strcat(str[4],", ");
											char s[15];
											sprintf(s,"%7lu",ppf.phone[i]);
											char ss[10];
											ss[0]=s[0]; ss[1]=s[1]; ss[2]=s[2]; ss[3]='-';
											ss[4]=s[3]; ss[5]=s[4]; ss[6]='-';
											ss[7]=s[5]; ss[8]=s[6]; ss[9]=0;
											strcat(str[4],ss+(ppf.phone[i]>999999l?0:1));
										}
									for (i=0; i<5; i++) {
										if (str[i][0])
											ff << str[i] << endl;
										if (i==1) { // output the price
											ff << "Цена: ";
											ff.setf(ios::fixed);
											ff.precision(2);
											ff << ppd.drec.price << ' ' <<
												ppi.paym[ppd.drec.pcode].abbr[0] <<
												ppi.paym[ppd.drec.pcode].abbr[1] <<
												ppi.paym[ppd.drec.pcode].abbr[2] <<
												ppi.paym[ppd.drec.pcode].abbr[3];
											Rates r;
											if (r.isvalid() && r[ppd.drec.pcode]
												&& r[ppd.drec.pcode]!=1) {
												ff << " (по курсу Пульса это ";
												ff.setf(ios::fixed);
												ff.precision(2);
#ifdef DEMO
												ff << " * DEMO * "<<
													" рублей)";
#else
												ff << r[ppd.drec.pcode]*ppd.drec.price <<
													" рублей)";
#endif
											}
											ff << endl;
										}
									}
								}
						Event *e=NULL;
						if (ll%100==50) {
							do {
								pb->setpos(ll);
								if (e)
									delete e;
								e=eq.get();
								if (e && e->type==evKeyPress) {
									KeyEvent *k=(KeyEvent*)e;
									if (k->scancode==S_ESC) {
										sb.working(FALSE);
										cw.Restore(buf);
										sb.say(s000);
										ff.close();
										if (fise)
											delete fise;
										return TRUE;
									}
								}
							} while (e);
							if (e)
								delete e;
						}
					}
				if (fise) {
					ff << "\nАдреса соответствующих фирм:\n";
					for (ll=0; ll<ppf.Nfirms(); ll++) {
						char s[15];
						char str[5][80];
						if (getbit(fise[(uint)(ll/32)],(uint)(ll%32))
								&& ppf.readn((uint)ll)) {
							if (!ppf.fullname || strlen(ppf.fullname)<2)
								sprintf(str[2],"Фирма: %s",ppf.firm);
							else {
								int ii=77-9-strlen(ppf.firm);
								if (strlen(ppf.fullname)>ii)
									ppf.fullname[ii]=0;
								sprintf(str[2],"Фирма: %s, %-s",ppf.firm,ppf.fullname);
							}
							sprintf(str[3],"Адрес: %s",ppf.address);
							sprintf(str[4],"Телефоны: (%03lu) ",ppf.area);
							for (int i=0; i<4; i++)
								if (ppf.phone[i]) {
									if (i==3)
										strcat(str[4],", Факс: ");
									else if (i)
										strcat(str[4],", ");
									sprintf(s,"%7lu",ppf.phone[i]);
									char ss[10];
									ss[0]=s[0]; ss[1]=s[1]; ss[2]=s[2]; ss[3]='-';
									ss[4]=s[3]; ss[5]=s[4]; ss[6]='-';
									ss[7]=s[5]; ss[8]=s[6]; ss[9]=0;
									strcat(str[4],ss+(ppf.phone[i]>999999l?0:1));
								}
							ff << endl;
							for (i=2; i<5; i++) {
								if (str[i][0])
									ff << str[i] << endl;
							}
						}
						Event *e=NULL;
						if (ll%100==50) {
							do {
								pb->setpos(ll);
								if (e)
									delete e;
								e=eq.get();
								if (e && e->type==evKeyPress) {
									KeyEvent *k=(KeyEvent*)e;
									if (k->scancode==S_ESC) {
										sb.working(FALSE);
										cw.Restore(buf);
										sb.say(s000);
										ff.close();
										if (fise)
											delete fise;
										return TRUE;
									}
								}
							} while (e);
							if (e)
								delete e;
						}
					}
				}
				cw.Restore(buf);
				sb.say(s000);
				sb.working(FALSE);
				ff.close();
            if (fise)
               delete fise;
            return TRUE;
         }
         }
      }
      return DataWindow::ChewMenuCommand(c);
   } catch (VArrayErr va) {
      if (va.code()==vaFWriteFault || va.code()==vaFCreateFault) {
         beep();
         MessageBox mb(" Ой ","Ошибка записи во временный файл.",
            "Проверьте наличие места, правильность настройки каталогов...");
         EventQueue eq;
         eq.setcommand(caCloseDataWin);
         return TRUE;
      } else
         throw va; // throw the error further
   }
}

void BaseWindow::draw() {                // draw the window
   if (lcur>iready) {
      lcur=iready;
      lfirst=(long)lcur-ch()>-1?lcur-ch()+1:0;
      moveto(lcur);
   }
   if (filtering) {
      SortOrder oldsort=srtorder;
      srtorder=srtNatural;
      Statusbar sb;
      char *s000=sb.get();
      { // handling filter
         sb.say("Идет фильтрация объявлений... Нажмите <ESC> для прерывания.");
         void *buf=NULL;
         Display d;
         ControlWindow cw(Rect(5,d.yMax()/2-2,75,d.yMax()/2+2),
            " Идет фильтрация объявлений... ",F_DOUBLE,TRUE,FALSE);
         PercentBar *pb=new PercentBar(2,1,65,0,100);
         cw.AddControl(pb);
         cw.Save(buf);
         cw.draw();
         EventQueue eq;
         sb.working(TRUE);
         if (!ppd.Bsize()) {
            if (ppd.setbuf(20000))
               data->SetBMax(data->BMax()-10);
         }
         long lor=lno;  // original lno
         while (iready<lno) {
            if (!ReadData(10)) {
               MessageBox("BaseWindow::draw()",
                  "Internal error, report to sg@mplik.ru",
                  "Reboot your computer to be safe");
               throw 1.1;
            }
            Event *e=NULL;
            do {
               if (e)
                  delete e;
               e=eq.get();
               pb->setpos((iready+lor-lno)*100/(lor));
               if (e && e->type==evKeyPress) {
                  KeyEvent *k=(KeyEvent*)e;
                  if (k->scancode==S_ESC) {
                     sb.working(FALSE);
                     cw.Restore(buf);
                     sb.say(s000);
                     eq.setcommand(caCloseDataWin);
                     if (ppd.Bsize())
                        data->SetBMax(data->BMax()+10);
                     ppd.delbuf();
                     return;
                  }
               }
            } while (e);
            delete e;
         }
         sb.working(FALSE);
         cw.Restore(buf);
         if (ppd.Bsize())
            data->SetBMax(data->BMax()+10);
         ppd.delbuf();
      }
      sb.say(s000);
      filtering=FALSE;
      if (lno==0) {
         beep();
         MessageBox(" Объявлений не найдено ",
            " Через Ваш фильтр не прошло ни одного объявления ",
            "Попробуйте ослабить фильтр");
         EventQueue eq;
         eq.setcommand(caCloseDataWin);
         return;
      }
      if (oldsort!=srtNatural) {
         srtorder=oldsort;
         iready=icur=0;
      }
   }  // filtering....
   if (sbtoggled) { // showbrief was just toggled
      sbtoggled=FALSE;
      resize(Rect(rect.x1,rect.y1,rect.x2,rect.y2+
         (showbrief?7:-7)));
      moveto(lcur);
   }
   int rep=redraw;
   DataWindow::draw();
   static long tglast;
   volatile long *time=(long*)0x46c;
   if (gfound!=-2) // if there's more than one group
      if (lcur==gfound) {
         VPutch(x1()+2,y2(),' ');
         VPuts(x1()+3,y2(),ppg.gname);
         int ii;
         VPutch(x1()+3+(ii=strlen(ppg.gname)),y2(),' ');
         VSetChar1(x1()+4+ii,y2(),48-ii,frame[6]);
         tglast=*time;
      } else {
         if (*time-tglast>10) // to prevent from blinking
            VSetChar1(x1()+2,y2(),50,frame[6]);
      }
   if (!showbrief) {
      mouse.hide();
      if (rep==255) {
         VClrFrame(x1(),y2()+1,cl(),5,' ',attr[attrAddrInfo]);
         VFrame(x1(),y2()+1,cl(),5,F_SINGLE,attr[attrAddrInfo]);
      }
      if (rep || lcur!=oldcur) { // need to reoutput the window
         oldcur=lcur;
         if (getfullinfo(lcur)) {
/*            char s[15];
				sprintf(s," %02d.%02d.%02d ",ppd.drec.dsubmit.day(),
					ppd.drec.dsubmit.month(),ppd.drec.dsubmit.yy());
            VPutsColor(68,rect.y2+1,s,attr[attrAddrData]);     */
            char str[5][80];
            strcpy(str[0],"Товар: ");
            strcpy(str[1],"       ");
            if (strlen(ppd.drec.ad)<68)
               strcat(str[0],ppd.drec.ad);
            else {
               for (char *c=ppd.drec.ad+68; *c!=' ' && c>ppd.drec.ad; c--)
                  if (c==ppd.drec.ad)
                     c+=68;
                  *c++=0;
               strcat(str[0],ppd.drec.ad);
               c[69]=0;
               strcat(str[1],c);
            }
            if (!ppf.fullname || strlen(ppf.fullname)<2)
               sprintf(str[2],"Фирма: %s",ppf.firm);
            else {
               int ii=77-9-strlen(ppf.firm);
               if (strlen(ppf.fullname)>ii)
                  ppf.fullname[ii]=0;
               sprintf(str[2],"Фирма: %s, %-s",ppf.firm,ppf.fullname);
            }
            sprintf(str[3],"Адрес: %s",ppf.address);
            sprintf(str[4],"Телефоны: (%03lu) ",ppf.area);
            for (int i=0; i<4; i++)
               if (ppf.phone[i]) {
                  if (i==3)
                     strcat(str[4],", Факс: ");
                  else if (i)
							strcat(str[4],", ");
						char s[15];
						sprintf(s,"%7lu",ppf.phone[i]);
                  char ss[10];
                  ss[0]=s[0]; ss[1]=s[1]; ss[2]=s[2]; ss[3]='-';
                  ss[4]=s[3]; ss[5]=s[4]; ss[6]='-';
                  ss[7]=s[5]; ss[8]=s[6]; ss[9]=0;
                  strcat(str[4],ss+(ppf.phone[i]>999999l?0:1));
               }
            for (i=0; i<5; i++) {
               int j=(i==4?9:6);
               VPutsColorLen(2+j,rect.y2+2+i,str[i]+j,
                  attr[attrAddrData],76-j);
               if (rep==255)
                  VPutsLen(2,rect.y2+2+i,str[i],j);
            }
         }
      }
      mouse.show();
   }
}

void BaseWindow::idlefunc() {
   try {
      if (iready+40<=lno)
         ReadData(40);
      else if (iready<lno)
         ReadData(1);
      else if (gfound!=-2 && gfound!=lcur) {
         // everything has been read, try to get group name
         long dd=(*data)(lcur) & 0x7fffffffl;
         if (ppg.issupply==supply &&
             ppi(ppg.gfirst,supply)<=dd &&
             ppi(ppg.gfirst+ppg.gsize,supply)>dd) {
             gfound=lcur;  // group found;
             draw();
            return; // already found
         }
         if (ppg.Gno()==ppg.Ngroups())
            ppg.rewind();
         ppg.read();
//         draw();
      }
   } catch (VArrayErr va) {
      if (va.code()==vaFWriteFault || va.code()==vaFCreateFault) {
         beep();
         MessageBox mb(" Ой ","Ошибка записи во временный файл.",
            "Проверьте наличие места, правильность настройки каталогов...");
         EventQueue eq;
         eq.setcommand(caCloseDataWin);
         return;
      } else
         throw va; // throw the error further
   }
}

BOOL BaseWindow::getfullinfo(long n) { // reads full info
   if (n>=lno) {
      throw 21;
   }
   if (n>=iready)
      if (!ReadData(n-iready+1))
         throw -1;
   if (!ppd.read((*data)(n) & 0x7fffffffl) || !ppf.readn(ppd.drec.fcode)) {
      MessageBox mb(" BaseWindow::GetLine ","Ошибка чтения файлов данных",
      "Файлы не найдены или повреждены...");
      return FALSE;
   }
   return TRUE;
}

char* BaseWindow::getline(long n) {      // return n-th line
	if (n>=lno)
		return "";
	if (n>=iready)
		if (!ReadData(n-iready+1))
			throw -1;
	if (!ppd.read((*data)(n) & 0x7fffffffl) || !ppf.readn(ppd.drec.fcode,TRUE)) {
		MessageBox mb(" BaseWindow::GetLine ","Ошибка чтения файлов данных",
		"Файлы не найдены или повреждены...");
		return "";
	}
	Rates rates;
	int sl=strlen(ppd.drec.ad);
	if (sl<70)
		setmem(ppd.drec.ad+sl,70-sl,' ');
	if (roubleprice && rates[ppd.drec.pcode])
		ppd.drec.price*=rates[ppd.drec.pcode];
	if (ppd.drec.price>1000 ||
		floor(ppd.drec.price*100)==floor(ppd.drec.price)*100)
#ifdef DEMO
		sprintf(ppd.drec.ad+50,"%10s","DEMO");
#else
		sprintf(ppd.drec.ad+50,"%10.0f",ppd.drec.price);
#endif
	else
#ifdef DEMO
		sprintf(ppd.drec.ad+50,"%10s","DEMO");
#else
		sprintf(ppd.drec.ad+50,"%10.2f",ppd.drec.price);
#endif
	ppd.drec.ad[60]=' '; // not zero, as it was set by sprintf
	if (roubleprice && rates[ppd.drec.pcode]) {
		char s[]="руб ";
		for (int i=0; i<4; i++)
         ppd.drec.ad[61+i]=s[i];
   } else
      for (int i=0; i<4; i++)
         ppd.drec.ad[61+i]=ppi.paym[ppd.drec.pcode].abbr[i];
   ppd.drec.ad[65]=' '; // not zero, as it was set by sprintf
   sprintf(ppd.drec.ad+66,"%-10s",ppf.firm);
   return ppd.drec.ad;

}

// *************************** SetRates ***************************************

SetRates::SetRates(Window *p) : savebuf(NULL),
   ListWindow(Rect(1,2,3,4),1," Установка курсов валют ",F_DOUBLE,TRUE,
      TRUE,p) {
   Rates rates;
   if (!rates.isvalid()) {
      beep();
      MessageBox mb("Ошибка открытия файла",
         "Не могу открыть файл pulse.ppi",
         "Проверьте правильность настройки каталогов");
      return;
   }
   setlno(rates.npaym);
   Display d;
   if (rates.npaym<d.yMax()-6)
      resize(Rect(14,(d.yMax()-rates.npaym)/2,66,
         (d.yMax()-rates.npaym)/2+rates.npaym+1));
   else
      resize(Rect(14,3,66,d.yMax()-2));
   Setup setup;
   BOOL opi=ppi.isopen();
   if (!opi)
      if (!ppi.open(setup.datapath))
         return;
   draw();
   GoModal("Корректировка пользовательских курсов валют");
   Restore(savebuf);
   if (!opi)
      ppi.close();
}; // only parent

void SetRates::GetRate(int n) {
   Statusbar sb;
   char *sbtext=sb.get();
   {
      void *buf=NULL;  // for saving screen
      char ratestr[10];
      char *s=ratestr;
      Display d;
      Rates rates;
      sprintf(ratestr,"%.2f",rates[n]);
      char titl[20];
      sprintf(titl," Новый курс '%c%c%c%c' ",ppi.paym[n].abbr[0],
         ppi.paym[n].abbr[1],ppi.paym[n].abbr[2],ppi.paym[n].abbr[3]);
      ControlWindow cw(Rect(25,d.yMax()/2-3,55,d.yMax()/2+3),
         titl,F_DOUBLE,TRUE,TRUE);
      InputField *ib=new InputField(3,1,9,s,
            (InputField::InpType)(InputField::it_float));
      // name, sel.char.pos, x,y , ncode, hotkey, def, saying, parent
      Button *b=new Button(" OK ",0,16,1,ncOkButton,0,TRUE);
      Button *cb=new Button(" Базовый ",3,16,3,ncCancButton,A_ALT_F,FALSE);
      ib->SetTTLRUD(b,cb);
      b->SetTTLRUD(cb,ib);
      cb->SetTTLRUD(ib,b);
      cw.AddControl(b);
      cw.AddControl(cb);
      cw.AddControl(ib);
      cw.Save(buf);
      cw.draw();
      char nc=cw.GoModal(
         "OK - ввод нового курса; Базовый - использование курса из базы");
      cw.Restore(buf);
      switch (nc) {
         case ncCloseButton:
            break;
         case ncCancButton:
            rates.clearuser(n);
            repaint(1);
            break;
         case ncOkButton: {
            char *es;
            float f=strtod(ratestr,&es);
            if (*es) {
               beep();
               MessageBox mb(" Ошибка ввода ",
                  "Число введено не верно",
                  "Убедитесь, что Ваше число не "
                  "содержит двух десятичных точек");
               break;
            }
            rates.setuser(n,f);
            repaint(1);
            break;
         }
         default:
            throw 22;
      }
   }
   sb.say(sbtext);
}

BOOL SetRates::selected(long l) {
   Rates rates;
   return rates.iscustom((int)l);
}

BOOL SetRates::ChewEvent(Event *e) {
   if (e->type==evKeyPress) {
      KeyEvent *k=(KeyEvent*)e;
      if (k->scancode==S_RET) {
         GetRate((int)lcur);
         return TRUE;
      }
   } else if (e->type==evMouseDblClick) {
      MouseEvent *m=(MouseEvent*)e;
      if (m->x>=cx1() && m->x<=cx2() && m->y<=cy2() && m->y>=cy1()) {
         GetRate((int)lcur);
         return TRUE;
      }
   }
   return ListWindow::ChewEvent(e);
}

void SetRates::draw() {
	mouse.hide();
	Save(savebuf);
	ListWindow::draw();
	mouse.show();
}

char* SetRates::getline(long n) {      // return n-th line
	Rates r;
	int i=(int)n;
	if (r.iscustom(i))
		sprintf(s,"  %c%c%c%c │ %8.2f │ (курс в базе: %8.2f)",ppi.paym[i].abbr[0],
			ppi.paym[i].abbr[1],ppi.paym[i].abbr[2],ppi.paym[i].abbr[3],
			r[i],ppi.paym[i].coef);
	else
		sprintf(s,"  %c%c%c%c │ %8.2f │ (используется курс из базы)",
			ppi.paym[i].abbr[0],ppi.paym[i].abbr[1],ppi.paym[i].abbr[2],
			ppi.paym[i].abbr[3],ppi.paym[i].coef);
	return s;
}


