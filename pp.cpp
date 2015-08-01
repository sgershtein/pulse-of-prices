//-----------------------------------------------------------------------------
// Prices' Pulse Viewer
// Ural-Relcom
// Sergey Gershtein
// Borland C++ Ver. 4.5
// Platform: DOS Standard; memory model: large (far code, far data, 64K static)
// Started: 30-Sep-95
//-----------------------------------------------------------------------------

#include "pp.h" // main include file
#include <string.h>
#include <except.h>
#include <stdio.h>
#include <alloc.h>

#define NOSAVEFILTER

//****************** Menu structure definition *************************

MenuStruct TOPMENU[] = {
   {" ���� ","���ଠ�� � �����, ��室 �� �ணࠬ��...",
    2,FALSE,FALSE,A_ALT_A, cmNone},
   {" ���� ","�롮� ���� ������",
    3,FALSE,FALSE,A_ALT_F, cmNone},
   {" �롮ઠ ","����樨 � ⥪�饩 �롮મ�",
    2,FALSE,FALSE,A_ALT_D, cmNone },
   {" ������ ","����� � 䨫��ࠬ�",
    3,FALSE,FALSE,A_ALT_B, cmNone },
   {" ����ன�� ","����ன�� �������㠫��� �।���⥭�� ���짮��⥫� ",
    2,FALSE,FALSE,A_ALT_Y, cmNone },
   {" ��ࠢ�� ","������� �ࠢ��� �����",
    2,FALSE,FALSE,A_ALT_C, cmNone } };

MenuStruct SUB_1[] = {
   {"���ଠ��  Ctrl-I","���ଠ�� � �����, ����� � �.�.",
   1,FALSE,FALSE,A_CTRL_I,cmBaseInfo },
   {"","",
   0,TRUE,TRUE,0,cmNone },  // delimiter
   {"��室       Alt-X","��室 �� �ணࠬ��",
   1,FALSE,FALSE,A_ALT_X,cmExit } };

MenuStruct SUB_2[] = {
   {"����       F5","���� ������ ��� �� ⮢��� � ��㣨",
   1,FALSE,FALSE,A_F5,cmDemand },
   {"�।������� F6","���� ������ �।������� ⮢�஢ � ���",
   1,FALSE,FALSE,A_F6,cmSupply },
   {"����       F7","���� ������ ��",
   1,FALSE,FALSE,A_F7, cmFirms}
   };

MenuStruct SUB_3[] = {
   {"�⬥��� ������     INS","�����/��⠭����� �⬥�� �� ⥪. ������",
   10,TRUE,FALSE,A_INS,cmSelect },
   {"�⬥��� ��      ��� +","�⬥��� �� �����",
   10,TRUE,FALSE,A_GREY_PL,cmSelectAll },
   {"����� �⬥��    ��� -","����� �⬥�� � ��� ����ᥩ",
   1,TRUE,FALSE,A_GREY_MI,cmDeselectAll },
   {"������ �⬥��  �஥ *",
      "����� �⬥�� � �⬥祭���, �⬥��� ���⬥祭��",
   1,TRUE,FALSE,A_GREY_MU,cmSelReverse },
   {"","",
   0,TRUE,TRUE,0,cmNone },  // delimiter
   {"����� �� �롮ન","����� ����祭�� ����� �� ⥪�饩 �롮ન",
   4,TRUE,FALSE,0,cmDelSel },
   {"������� � 䠩�   Ctrl-F2",
      "������� ����祭�� ����� � 䠩� ��� �ᯥ����",
   12,TRUE,FALSE,A_CTRL_F2,cmSaveCur },
/*   {"����⨪�","�������� ����⨪� �� ⥪�饩 �롮થ",
   1,TRUE,FALSE,0,cmStat }, */
   {"","",
   0,TRUE,TRUE,0,cmNone },  // delimiter
   {"���� ��ப�      Ctrl-S","�᪠�� �����ப� � ⥪�饩 �롮થ",
   1,TRUE,FALSE,A_CTRL_S,cmFind },
   {"������� ����   Ctrl-L","���� ᫥���饥 �宦����� ��ப� � �롮થ",
   1,TRUE,FALSE,A_CTRL_L,cmFindNext },

};

MenuStruct SUB_4[] = {
   {"�������/��������    F4","������� ��� ��।���஢��� 䨫���",
   1,FALSE,FALSE,A_F4,cmEditFilter },
//   {"����㧨��           F3","����㧨�� ��⮢� 䨫��� � ��᪠",
//   1,TRUE/*FALSE*/,FALSE,A_F3,cmLoadFilter },
//   {"���࠭���           F2","���࠭��� ⥪�騩 䨫��� �� ���",
//   3,TRUE,FALSE,A_F2,cmSaveFilter },
   {"�ਬ����� 䨫���  Ctrl-F",
      "�����뢠�� �� ���� ⮫쪮 ��䨫��஢���� �����",
   3,TRUE,FALSE,A_CTRL_F,cmApplyFilter },
   {"","",
   0,TRUE,TRUE,0,cmNone },  // delimiter
   {"�� ���஢���","�뢮� ������ � ��室��� ���浪�",
   1,FALSE,TRUE,0,cmNoSort },
/*   {"����஢�� �� ⮢���","�����஢��� ⮢��� �� ��䠢���",
   15,FALSE,FALSE,0,cmSortGoods }, */
   {"����஢�� �� �����⠭�� 業",
      "�����஢��� ����� �� �����⠭�� 業",
   15,FALSE,FALSE,0,cmSortPrices },
   {"����஢�� �� �뢠��� 業",
    "�����஢��� ����� �� �뢠��� 業",
   15,FALSE,FALSE,0,cmSortPricesDesc },
   };

MenuStruct SUB_5[] = {
/*   {"��⠫���","����ன�� ��⠫���� ��� ��� ������, 䨫��஢ � �.�.",
   1,TRUE,FALSE,0,cmDirSetup },
   {"�।���⥭��","����ன�� ���짮��⥫�᪨� �।���⥭��",
   1,TRUE,FALSE,0,cmUserSetup }, */
   {"����� �����  F9","���४�஢�� ���ᮢ ����� ��� ���� 業",
   7,FALSE,FALSE,A_F9,cmCurrency },
   {"","",
   0,TRUE,TRUE,0,cmNone },  // delimiter
   {"��⪠� �ଠ ","�����뢠�� ������ ��������� ��� ᮪�饭��",
   1,FALSE,TRUE,0,cmShowBrief },
   {"���� � �㡫��  Ctrl-R","�����뢠�� �� 業� � �㡫�� �� �����",
   8,FALSE,FALSE,A_CTRL_R
   ,cmRoublePrice },
/*   {"","",
   0,TRUE,TRUE,0,cmNone },  // delimiter
   {"���࠭���","���࠭��� ��⠭���� ��� �ᯮ�짮����� � ᫥���騩 ࠧ",
   1,TRUE,FALSE,0,cmSaveOptions }, */
   };

MenuStruct SUB_6[] = {
   {"� �ணࠬ��","��ࠢ�� � �ணࠬ�� ���� ���",
   3,FALSE,FALSE,0,cmAbout },
   {"�ࠫ-������  F1","���⠪�� ���न���� �ࠫ-������",
   3,FALSE,FALSE,A_F1,cmRelcom },
   };

//****************** end of Menu structure definition ***********************

// Global handlers
Display *display;    // display handler
Mouse *mouse;		 	// mouse handler
Keyboard *keyboard;  // keyboard handler
EventQueue *evlist;	// input events lister
Cursor cursor;       // cursor management
IOHardErr ioharderr; // Hard I/O Error management
CtrlBreak cbreak;    // ctrl-break management
DataWindow *datawin=NULL; // Main data window
int oldwin=0; // 1 - groups supply, 2 - groups demand , 3 - firms, 0 -none
long oldcur;
FilterWindow filter; // filter window

int main(int argc, char *argv[]) {	// trial run version
	// Initializing global handlers
	try {
		if (argc>1) { // checking for key supplied
			if (argv[1][0]=='/' && upcase(argv[1][1])=='K' && argv[1][2]=='=') {
				basekey.set(atol(argv[1]+3));
				strcpy(basekey.username,"--none--");
			}
		}
		char ipath[70];
		strcpy(ipath,argv[0]);
		for (char *i=ipath+(strlen(ipath)-1); *i!='\\'; i--);
		*++i=0; // remove the program name
		Setup setup(ipath);
		setup.read(Setup::all);
#ifndef DEMO
		KeyCheck PulseKey; 			// checking keys
		PulseKey.FindKey();			// read keys & compare; throws float if fail
#endif
		display = new Display;
		if (setup.hires)
			display->HiRes();
      mouse = new Mouse(display);
      mouse->Init();
      if (!setup.grmousecur)
         mouse->Emulation(FALSE);
      mouse->LeftHand(setup.lefthandmouse);
      mouse->moveto(40,12);
      evlist=new EventQueue;
      evlist->addhandler(mouse);
      keyboard=new Keyboard;
      evlist->addhandler(keyboard);
      Background bk(setup.bchar);
      Statusbar st;
      // pull-down menus initialization
      PullDnMenu *mpd[sizeof(TOPMENU)/sizeof(MenuStruct)]={
         new PullDnMenu(sizeof(SUB_1)/sizeof(MenuStruct),SUB_1),
         new PullDnMenu(sizeof(SUB_2)/sizeof(MenuStruct),SUB_2),
         new PullDnMenu(sizeof(SUB_3)/sizeof(MenuStruct),SUB_3),
         new PullDnMenu(sizeof(SUB_4)/sizeof(MenuStruct),SUB_4),
         new PullDnMenu(sizeof(SUB_5)/sizeof(MenuStruct),SUB_5),
         new PullDnMenu(sizeof(SUB_6)/sizeof(MenuStruct),SUB_6)};
      // creating the top menu with all the structure
      TopMenu mtop(sizeof(TOPMENU)/sizeof(MenuStruct),TOPMENU,mpd,4);
      cursor.hide();
		bk.draw();     // show background
		drawuser();
      mtop.draw();   // show menu
		st.say("���� ���������� � ���� ���!  �室 � ���� - ������ F10");
#ifndef DEMO
		PulseKey.check();
		if (!PulseKey.warnCIC())
		 {AboutBox ab(TRUE);}
#else
		{AboutBox ab(TRUE);}
#endif
		BOOL firstime=TRUE;
		for (;;) {
			Event *e=NULL;
			char cm; // command
			do {
				if (firstime && !e) {
					firstime=FALSE;
					e=new KeyEvent(A_F6 >> 8, 0);
					continue;
				}
				char i;  // local loop variable
				cm=evlist->getcommand();
				switch (cm) {
				case cmNone:
					break;
				case cmExit:
					mouse->Restore();
					if (datawin)
						delete datawin;
					display->Done("���ᨡ� �� ࠡ��� � ���ᮬ ���!");
					delete evlist;
					delete display;
					for (i=0; i<sizeof(TOPMENU)/sizeof(MenuStruct); i++)
						delete mpd[i]; // deleting menu structure
					return 0;
				case cmAbout:
					{AboutBox ab;}
					break;
				case cmRelcom:
					{RelcomInfo ri;}
					break;
				case cmBaseInfo:
					{BaseInfoBox ib;}
					break;
				case cmFirms:
					if (datawin) {
						delete datawin;
						bk.draw();
						drawuser();
						mtop.draw();
						st.draw();
               }
               datawin = new FirmWindow(&mtop);
               if (!datawin->valid()) { // any error
                  delete datawin;
                  datawin=NULL;
               }
               oldwin=0;
               break;
            case cmSupply:
            case cmDemand:
               if (datawin) {
                  delete datawin;
						bk.draw();
						drawuser();
						mtop.draw();
                  st.draw();
               }
               datawin = new GroupWindow(&mtop,cm==cmSupply);
					if (!datawin->valid()) { // any error
                  delete datawin;
                  datawin=NULL;
               }
               oldwin=0;
               break;

            case cmNoSort:
            case cmSortPrices:
            case cmSortPricesDesc:
               if (!datawin || !datawin->ChewMenuCommand(cm)) {
                  if (cm!=cmNoSort)
                     mtop.SetChecked(FALSE,cmNoSort);
                  if (cm!=cmSortPrices)
                     mtop.SetChecked(FALSE,cmSortPrices);
                  if (cm!=cmSortPricesDesc)
                     mtop.SetChecked(FALSE,cmSortPricesDesc);
                  mtop.SetChecked(TRUE,cm);
					}
               break;
            case cmShowBrief:
            case cmRoublePrice:
            case cmApplyFilter:
					if (!datawin || !datawin->ChewMenuCommand(cm)) {
						mtop.ToggleChecked(cm);
						MessageBox(" ������ ",mtop.IsChecked(cm)?
                    "������ ��⨢��":"������ ����⨢��",
                    "����祭��/�몫�祭�� 䨫��� - Ctrl-F");
					}
					break;
				case cmCurrency: {
					SetRates sr;
				}
				break;
				case cmEditFilter: {
					filter.repaint(255);
					filter.edit();
					if (datawin && datawin->which()==DataWindow::base) {
						if (mtop.IsChecked(cmApplyFilter))
							datawin->ChewMenuCommand(cmApplyFilter); // clear Apply filter
					}
					if (filter.isempty()) {
						mtop.SetChecked(FALSE,cmApplyFilter);
						mtop.SetDisabled(TRUE,cmApplyFilter);
					} else
						mtop.SetDisabled(FALSE,cmApplyFilter);
					bk.draw();
					drawuser();
					mtop.draw();
					st.draw();
					if (datawin)
						datawin->repaint(255);
					if (!filter.isempty() &&
						(!datawin || datawin->which()!=DataWindow::base))
							mtop.SetChecked(TRUE,cmApplyFilter);
//					mtop.SetDisabled(filter.isempty(),cmSaveFilter);
/*					MessageBox(" ������ ",mtop.IsChecked(cm)?
						"������ ��⨢��":"������ ����⨢��",
						"����祭��/�몫�祭�� 䨫��� - Ctrl-F"); */
					break;
				}
#ifndef NOSAVEFILTER
				case cmSaveFilter:
					if (!filter.isempty())
						filter.write();
					break;
				case cmLoadFilter:
					filter.manage();
					mtop.SetDisabled(filter.isempty(),cmSaveFilter);
					break;
#endif
// ------------------------- now actions, not menu commands -----------------
            case caCloseDataWin:
               if (datawin) {
						bk.draw();
						drawuser();
                  mtop.draw();
                  st.draw();
                  if (datawin->which()==DataWindow::base && oldwin) {
                     delete datawin;
                     if (oldwin<3)
								datawin=new GroupWindow(&mtop,oldwin==1);
                     else
                        datawin=new FirmWindow(&mtop);
                     datawin->moveto(oldcur);
                     oldwin=0;
                  } else {
                     delete datawin;
                     datawin=NULL;
                  }
               }
               break;
            case caOpenBaseWin:
               oldcur=datawin->Lcur();
               if (datawin->which()==DataWindow::groups) {
                  oldwin=mtop.IsChecked(cmSupply)?1:2;
                  GroupWindow *dw=(GroupWindow*)datawin;
                  datawin = new BaseWindow(&mtop,dw,&filter);
                  // dw will be destroyed inside of BaseWindow constructor.
					} else if (datawin->which()==DataWindow::firms) {
                  oldwin=3;
						bk.draw();
						drawuser();
                  mtop.draw();
                  st.draw();
                  FirmWindow *fw=(FirmWindow*)datawin;
                  datawin = new BaseWindow(&mtop,fw,&filter);
                  // fw will be destroyed inside of BaseWindow constructor.
               }
               if (e)
                  delete e;
               e=NULL; continue;
				default:
					if (cm>caNone) {
						beep();
						MessageBox(" ����७��� �訡��! ",
						"����� �� ����ࠡ��뢠���� ����⢨�",
						"�������� ᮮ��� ࠧࠡ��稪� ����!");
					} else if (!datawin || !datawin->ChewMenuCommand(cm)) {
						beep();
						MessageBox(" �������! ",
						"������ ���� �� ॠ��������","");
					}
				} // switch
#ifndef DEMO
				PulseKey.check();
#endif
				for(;;) {
					if (e)
						delete e;
					if (evlist->iscommand()) {
						e=NULL;
						break;
					}
					e=evlist->get();
					KeyEvent *k;
					if (e && e->type==evKeyPress)
						k=(KeyEvent*)e;
					else
						k=NULL;
					if ((k && (k->code==0)) || cbreak.flag()) {
						beep();
						MessageBox mb (" ��祬 �� ⠪? ",
		"��� ��室� ������ Alt-X, Ctrl-Break �ਤ㬠� �� ��� �⮣�.");
						cbreak.clear();
						continue;
					}
					if (e)
						break;
					else if (datawin)
						datawin->idlefunc();
					else if (heapcheck()==_HEAPCORRUPT) {
						MessageBox(" ����襭�� �������� �������� ����� ",
							" ����襭� ������� ����⨢��� "
							"����� ����樮���� ��⥬�",
							"��������㥬 ��१���㧨�� ��������"
							" ��᫥ ��室� �� ����!");
						throw 1.1;
					}
				}
			} while ((mtop.ChewEvent(e)) || (datawin && datawin->ChewEvent(e)));
#ifndef DEMO
			PulseKey.check();
#endif
			if (e->type==evKeyPress) {
				KeyEvent *k=(KeyEvent*)e;
				if (k->keycode==K_ESC && setup.easyexit)
					evlist->setcommand(cmExit);
			}
		}
	} catch (xalloc) {
		MessageBox(" �訡�� �뤥����� ����� ",
			" ��������� ����⨢��� ����� �� �����筮 ��� ࠡ��� �ணࠬ��",
			"��������㥬 ��१���㧨�� �������� ��᫥ ��室� �� ����!");
	} catch (double) {
		// do nothing
	} catch (int i) {
		char str[60];
		sprintf(str," ����७��� �訡�� #%d ",i);
		MessageBox(str,
			"�������� ᮮ��� ࠧࠡ��稪� ���� ����� �訡��!",
			"��᫥ ��室� �� �ணࠬ�� ����������� ��१���㧨� ��������!");
	} catch (...) {
		MessageBox(" ����।�ᬮ�७��� �᪫�祭��! ",
			" �������⭠� �訡��. �������� ᮮ��� ࠧࠡ��稪� ����!",
			"��᫥ ��室� �� �ணࠬ�� ����������� ��१���㧨� ��������!");
	}
	return 1;
}
