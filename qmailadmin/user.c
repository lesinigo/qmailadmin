/* 
 * $Id: user.c,v 1.11.2.4 2004-11-14 18:05:55 tomcollins Exp $
 * Copyright (C) 1999-2002 Inter7 Internet Technologies, Inc. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <errno.h>
#include <vpopmail_config.h>
/* undef some macros that get redefined in config.h below */
#undef PACKAGE_NAME  
#undef PACKAGE_STRING 
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include "config.h"
#include "qmailadmin.h"
#include "qmailadminx.h"
#include "vpopmail.h"
#include "vauth.h"


#define HOOKS 1

#ifdef HOOKS
/* note that as of December 2003, only the first three hooks are 
implemented */
#define HOOK_ADDUSER     "adduser"
#define HOOK_DELUSER     "deluser"
#define HOOK_MODUSER     "moduser"
#define HOOK_ADDMAILLIST "addmaillist"
#define HOOK_DELMAILLIST "delmaillist"
#define HOOK_MODMAILLIST "modmaillist"
#define HOOK_LISTADDUSER "addlistuser"
#define HOOK_LISTDELUSER "dellistuser"
#endif

int show_users(char *Username, char *Domain, time_t Mytime)
{
  if (MaxPopAccounts == 0) return 0;
  send_template("show_users.html");
}


int show_user_lines(char *user, char *dom, time_t mytime, char *dir)
{
 int  i,j,k,startnumber,moreusers = 1;
 FILE *fs;
 struct vqpasswd *pw;
 int totalpages;
 int bounced;
 int colspan = 7;
 int allowdelete;
 char qconvert[11];

  if (MaxPopAccounts == 0) return 0;

  /* Get the default catchall box name */
  if ((fs=fopen(".qmail-default","r")) == NULL) {
    /* report error opening .qmail-default and exit */
    printf ("<tr><td colspan=\"%i\">%s .qmail-default</tr></td>", 
      colspan, get_html_text("144"));
    vclose();
    exit(0);
  }

  fgets(TmpBuf, sizeof(TmpBuf), fs);
  fclose(fs);

  if (*SearchUser) {
    pw = vauth_getall(dom,1,1);
    for (k=0; pw!=NULL; k++) {
      if ((!SearchUser[1] && *pw->pw_name >= *SearchUser) ||
          ((!strcmp(SearchUser, pw->pw_name)))) {
        break;
      }

      pw = vauth_getall(dom,0,0);
    }

    if (k == 0) strcpy (Pagenumber, "1");
    else sprintf(Pagenumber, "%d", (k/MAXUSERSPERPAGE)+1);
  }

  /* Determine number of pages */
  pw = vauth_getall(dom,1,1);
  for (k=0; pw!=NULL; k++) pw = vauth_getall(dom, 0, 0);

  if (k == 0) totalpages = 1;
  else totalpages = ((k/MAXUSERSPERPAGE)+1);

  /* End determine number of pages */
  if (atoi(Pagenumber)==0) *Pagenumber='1';

  if ( strstr(TmpBuf, " bounce-no-mailbox\n") != NULL ) {
    bounced = 1;
  } else if ( strstr(TmpBuf, "@") != NULL ) {
    bounced = 0;
    /* check for local user to forward to */
    if (strstr(TmpBuf, dom) != NULL) {
      i = strlen(TmpBuf); --i; TmpBuf[i] = 0; /* take off newline */
      for(;TmpBuf[i]!=' ';--i);
      for(j=0,++i;TmpBuf[i]!=0 && TmpBuf[i]!='@';++j,++i) 
         TmpBuf3[j] = TmpBuf[i];
      TmpBuf3[j]=0;
    }
  } else {
    /* Maildir type catchall */
    bounced = 0;
    i = strlen(TmpBuf); --i; TmpBuf[i] = 0; /* take off newline */
    for(;TmpBuf[i]!='/';--i);
    for(j=0,++i;TmpBuf[i]!=0;++j,++i) TmpBuf3[j] = TmpBuf[i];
    TmpBuf3[j]=0;
  }

  startnumber = MAXUSERSPERPAGE * (atoi(Pagenumber) - 1);

  /*
   * check to see if there are any users to list, 
   * otherwise repeat previous page
   *  
   */
  pw = vauth_getall(dom,1,1);
  if ( AdminType==DOMAIN_ADMIN ||
       (AdminType==USER_ADMIN && strcmp(pw->pw_name,Username)==0)){

    for (k = 0; k < startnumber; ++k) { 
      pw = vauth_getall(dom,0,0); 
    }
  }

  if (pw == NULL) {
    printf ("<tr><td colspan=\"%i\" bgcolor=%s>%s</td></tr>\n", 
      colspan, get_color_text("000"), get_html_text("131"));
      moreusers = 0;
    } else {
      char path[256];
      while ((pw != NULL) && ((k < MAXUSERSPERPAGE + startnumber) ||  
              (AdminType!=DOMAIN_ADMIN || AdminType!=DOMAIN_ADMIN || 
              (AdminType==USER_ADMIN && strcmp(pw->pw_name,Username)==0)))) {
        if (AdminType==DOMAIN_ADMIN || 
            (AdminType==USER_ADMIN && strcmp(pw->pw_name,Username)==0)) {
          long diskquota = 0, maxmsg = 0;

          /* display account name and user name */
          printf ("<tr bgcolor=%s>", get_color_text("000"));
          printh ("<td align=\"left\">%H</td>", pw->pw_name);
          printh ("<td align=\"left\">%H</td>", pw->pw_gecos);

          /* display user's quota */
	  snprintf(path, sizeof(path), "%s/" MAILDIR, pw->pw_dir);
          readuserquota(path, &diskquota, &maxmsg);
          printf ("<td align=\"right\">%-2.2lf&nbsp;/&nbsp;</td>", ((double)diskquota)/1048576.0);  /* Convert to MB */
          if (strncmp(pw->pw_shell, "NOQUOTA", 2) != 0) {
              if(quota_to_megabytes(qconvert, pw->pw_shell)) {
                  printf ("<td align=\"left\">(BAD)</td>");
              }
              else { printf ("<td align=\"left\">%s</td>", qconvert); }
          }
          else { printf ("<td align=\"left\">%s</td>", get_html_text("229")); }

          /* display button to modify user */
          printf ("<td align=\"center\">");
          printh ("<a href=\"%s/com/moduser?user=%C&dom=%C&time=%d&moduser=%C\">",
            CGIPATH,user,dom,mytime,pw->pw_name);
          printf ("<img src=\"%s/modify.png\" border=\"0\"></a>", IMAGEURL);
          printf ("</td>");
            
          /* if the user has admin privileges and pw->pw_name is not 
           * the user or postmaster, allow deleting 
           */
          if (AdminType==DOMAIN_ADMIN && 
               strcmp(pw->pw_name, Username) != 0 &&
               strcmp(pw->pw_name, "postmaster") != 0) {
            allowdelete = 1;

          /* else, don't allow deleting */
          } else {
            allowdelete = 0;
          }

          /* display trashcan for delete, or nothing if delete not allowed */
          printf ("<td align=\"center\">");
          if (allowdelete) {
            printh ("<a href=\"%s/com/deluser?user=%C&dom=%C&time=%d&deluser=%C\">",
              CGIPATH,user,dom,mytime,pw->pw_name);
            printf ("<img src=\"%s/trash.png\" border=\"0\"></a>", IMAGEURL);
          } else {
            /* printf ("<img src=\"%s/disabled.png\" border=\"0\">", IMAGEURL); */
          }
          printf ("</td>");

          /* display button in the 'set catchall' column */
          printf ("<td align=\"center\">");
          if (bounced==0 && strncmp(pw->pw_name,TmpBuf3,sizeof(TmpBuf3)) == 0) {
            printf ("<img src=\"%s/radio-on.png\" border=\"0\"></a>", 
              IMAGEURL);
          } else if (AdminType==DOMAIN_ADMIN) {
            printh ("<a href=\"%s/com/setdefault?user=%C&dom=%C&time=%d&deluser=%C&page=%s\">",
              CGIPATH,user,dom,mytime,pw->pw_name,Pagenumber);
            printf ("<img src=\"%s/radio-off.png\" border=\"0\"></a>",
              IMAGEURL);
          } else {
            printf ("<img src=\"%s/disabled.png\" border=\"0\">",
              IMAGEURL);
          }
          printf ("</td>");

          printf ("</tr>\n");
        }        
        pw = vauth_getall(dom,0,0);
        ++k;
      }
    }

    if (AdminType == DOMAIN_ADMIN) {
#ifdef USER_INDEX
      printf ("<tr bgcolor=%s>", get_color_text("000"));
      printf ("<td colspan=\"%i\" align=\"center\">", colspan);
      printf ("<hr>");
      printf ("<b>%s</b>", get_html_text("133"));
      printf ("<br>");
      for (k = 'a'; k <= 'z'; k++) {
        printh ("<a href=\"%s/com/showusers?user=%C&dom=%C&time=%d&searchuser=%c\">%c</a>\n",
          CGIPATH,user,dom,mytime,k,k);
      }
      printf ("<br>");
      for (k = 0; k < 10; k++) {
        printh ("<a href=\"%s/com/showusers?user=%C&dom=%C&time=%d&searchuser=%d\">%d</a>\n",
          CGIPATH,user,dom,mytime,k,k);
      }
      printf ("</td>");
      printf ("</tr>\n");

      printf ("<tr bgcolor=%s>", get_color_text("000"));
      printf ("<td colspan=%i>", colspan);
      printf ("<table border=0 cellpadding=3 cellspacing=0 width=\"100%%\"><tr><td align=\"center\"><br>");
      printf ("<form method=\"get\" action=\"%s/com/showusers\">", CGIPATH);
      printh ("<input type=\"hidden\" name=\"user\" value=\"%H\">", user);
      printh ("<input type=\"hidden\" name=\"dom\" value=\"%H\">", dom);
      printf ("<input type=\"hidden\" name=\"time\" value=\"%d\">", mytime);
      printh ("<input type=\"text\" name=\"searchuser\" value=\"%H\">&nbsp;", SearchUser);
      printf ("<input type=\"submit\" value=\"%s\">", get_html_text("204"));
      printf ("</form>");
      printf ("</td></tr></table>");
      printf ("<hr>");
      printf ("</td></tr>\n");
#endif

      printf ("<tr bgcolor=%s>", get_color_text("000"));
      printf ("<td colspan=\"%i\" align=\"right\">", colspan);
#ifdef USER_INDEX
      printf ("<font size=\"2\"><b>");
      printf ("[&nbsp;");
      /* only display "previous page" if pagenumber > 1 */
      if (atoi(Pagenumber) > 1) {
        printh ("<a href=\"%s/com/showusers?user=%C&dom=%C&time=%d&page=%d\">%s</a>",
          CGIPATH,user,dom,mytime,
          atoi(Pagenumber)-1 ? atoi(Pagenumber)-1 : atoi(Pagenumber), 
          get_html_text("135"));
        printf ("&nbsp;|&nbsp");
      }

      if (moreusers && atoi(Pagenumber) < totalpages) {
        printh ("<a href=\"%s/com/showusers?user=%C&dom=%C&time=%d&page=%d\">%s</a>",
          CGIPATH,user,dom,mytime,atoi(Pagenumber)+1,
          get_html_text("137"));
        printf ("&nbsp;|&nbsp");
      }
/*        printf ("&nbsp;|&nbsp");*/
#endif
      printh ("<a href=\"%s/com/deleteall?user=%C&dom=%C&time=%d\">%s</a>", 
        CGIPATH,user,dom,mytime,get_html_text("235"));
      printf ("&nbsp;|&nbsp");
      printh ("<a href=\"%s/com/bounceall?user=%C&dom=%C&time=%d\">%s</a>", 
        CGIPATH,user,dom,mytime,get_html_text("134"));
      printf ("&nbsp;|&nbsp");
      printh ("<a href=\"%s/com/setremotecatchall?user=%C&dom=%C&time=%d\">%s</a>", 
        CGIPATH,user,dom,mytime,get_html_text("206"));
      printf ("&nbsp]");
      printf ("</b></font>");
      printf ("</td></tr>\n");
  }
  return 0;
}

adduser()
{
  count_users();
  load_limits();

  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }
                                                
  if ( MaxPopAccounts != -1 && CurPopAccounts >= MaxPopAccounts ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s %d\n", get_html_text("199"),
      MaxPopAccounts);
    show_menu();
    vclose();
    exit(0);
  }

  send_template( "add_user.html" );

}

moduser()
{
  if (!( AdminType==DOMAIN_ADMIN ||
        (AdminType==USER_ADMIN && strcmp(ActionUser,Username)==0))){
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }
  send_template( "mod_user.html" );
} 

addusernow()
{
 char pw[50];
 int cnt=0, num;
 char *c_num;
 char **mailingListNames;
 char *tmp;
 char *email;
 char **arguments;
 char tmpstr[MAX_BUFF];
 char qconvert[11];
 int pid;
 int i;
 int tmpint;
 int error;
 struct vqpasswd *mypw;
 char pw_shell[256];
 char spamvalue[50];
 static char NewBuf[156];
 FILE *fs;

  c_num = malloc(MAX_BUFF);
  email = malloc(128);
  tmp = malloc(MAX_BUFF);
  arguments = (char **)malloc(MAX_BUFF);

  count_users();
  load_limits();

  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }

  if ( MaxPopAccounts != -1 && CurPopAccounts >= MaxPopAccounts ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s %d\n", get_html_text("199"),
      MaxPopAccounts);
    show_menu();
    vclose();
    exit(0);
  }
 
  GetValue(TmpCGI,Newu, "newu=", sizeof(Newu));

  if ( fixup_local_name(Newu) ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s %s\n", get_html_text("148"), Newu);
    adduser();
    vclose();
    exit(0);
  } 

  if ( check_local_user(Newu) ) {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("175"), Newu);
    adduser();
    vclose();
    exit(0);
  } 
  // Coded added by jhopper
#ifdef MODIFY_QUOTA
  GetValue(TmpCGI, Quota, "quota=", sizeof(Quota));
#endif
  GetValue(TmpCGI,Password1, "password1=", sizeof(Password1));
  GetValue(TmpCGI,Password2, "password2=", sizeof(Password2));
  if ( strcmp( Password1, Password2 ) != 0 ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("200"));
    adduser();
    vclose();
    exit(0);
  }

#ifndef ENABLE_LEARN_PASSWORDS
  if ( strlen(Password1) <= 0 ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("234"));
    adduser();
    vclose();
    exit(0);
  }
#endif

  snprintf (email, 128, "%s@%s", Newu, Domain);
    
  GetValue(TmpCGI,Gecos, "gecos=", sizeof(Gecos));
  if ( strlen( Gecos ) == 0 ) {
    strcpy(Gecos, Newu);
  }

  GetValue(TmpCGI, c_num, "number_of_mailinglist=", MAX_BUFF);
  num = atoi(c_num);
  if(!(mailingListNames = malloc(sizeof(char *) * num))) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("201"));
    vclose();
    exit(0);

  } else {
    for(cnt = 0; cnt < num; cnt++) {
      if(!(mailingListNames[cnt] = malloc(MAX_BUFF))) {
        snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("201"));
        vclose();
        exit(0);
      }
    }

    for(cnt = 0; cnt < num; cnt++) {
      sprintf(tmp, "subscribe%d=", cnt);
      error = GetValue(TmpCGI, mailingListNames[cnt], tmp, MAX_BUFF);
      if( error != -1 ) {
        pid=fork();

        if (pid==0) {
          sprintf(TmpBuf1, "%s/ezmlm-sub", EZMLMDIR);
          sprintf(TmpBuf2, "%s/%s", RealDir, mailingListNames[cnt]);
          execl(TmpBuf1, "ezmlm-sub", TmpBuf2, email, NULL);
          exit(127);
        } else {
          wait(&pid);
        }
      } 
    }
  }

  /* add the user then get the vpopmail password structure */
  if ( vadduser( Newu, Domain, Password1, Gecos, USE_POP ) == 0 && 
#ifdef MYSQL_REPLICATION
    !sleep(2) &&
#endif
    (mypw = vauth_getpw( Newu, Domain )) != NULL ) {

    /* vadduser() in vpopmail 5.3.29 and later sets the default
     * quota, so we only need to change it if the user enters
     * something in the Quota field.
     */

#ifdef MODIFY_QUOTA
    if (strcmp (Quota, "NOQUOTA") == 0) {
      vsetuserquota (Newu, Domain, "NOQUOTA");
    } else if ( Quota[0] != 0 ) {
      if(quota_to_bytes(qconvert, Quota)) { 
        snprintf (StatusMessage, sizeof(StatusMessage), get_html_text("314"));
      } else {
        vsetuserquota (Newu, Domain, qconvert);
      }
    }
#endif

#ifdef MODIFY_SPAM
    GetValue(TmpCGI, spamvalue, "spamcheck=", sizeof(spamvalue));
    if(strcmp(spamvalue, "on") == 0) {
       snprintf(NewBuf, sizeof(NewBuf), "%s/.qmail", mypw->pw_dir);
       fs = fopen(NewBuf, "w+");
       fprintf(fs, "%s\n", SPAM_COMMAND);
       fclose(fs);
    }
#endif

    /* report success */
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H@%H (%H) %s",
      get_html_text("002"), Newu, Domain, Gecos,
      get_html_text("119"));

  } else {
    /* otherwise, report error */
    snprinth (StatusMessage, sizeof(StatusMessage), "<font color=\"red\">%s %H@%H (%H) %s</font>", 
      get_html_text("002"), Newu, Domain, Gecos, get_html_text("120"));
  }

  call_hooks(HOOK_ADDUSER, Newu, Domain, Password1, Gecos);

  /* After we add the user, show the user page
   * people like to visually verify the results
   */
  show_users(Username, Domain, Mytime);

}

int call_hooks(char *hook_type, char *p1, char *p2, char *p3, char *p4)
{
 FILE *fs = NULL;
 int pid;
 char *hooks_path;
 char *cmd = NULL;
 char *tmpstr;
 int error;
    
  hooks_path = malloc(MAX_BUFF);

  /* first look in directory for domain */
  sprintf(hooks_path, "%s/.qmailadmin-hooks", RealDir);
  if((fs = fopen(hooks_path, "r")) == NULL) {
    /* then try ~vpopmail/etc */
    sprintf(hooks_path, "%s/etc/.qmailadmin-hooks", VPOPMAILDIR);
    if((fs = fopen(hooks_path, "r")) == NULL) {
      return (0);
    }
  }

  while(fgets(TmpBuf, sizeof(TmpBuf), fs) != NULL) {
    if ( (*TmpBuf == '#') || (*TmpBuf == '\0')) continue;
    tmpstr = strtok(TmpBuf, " :\t\n");
    if (tmpstr == NULL) continue;

    if ( strcmp(tmpstr, hook_type) == 0) {
      tmpstr = strtok(NULL, " :\t\n");

      if ((tmpstr == NULL) || (*tmpstr == '\0')) continue;

      cmd = strdup(tmpstr);        

      break;
    }
  }

  fclose(fs);

  if (cmd == NULL) return 0;   /* don't have a hook for this type */
    
  pid = fork();

  if (pid == 0) {
    /* Second param to execl should actually be just the program name,
       without the path information.  Add a pointer to point into cmd
       at the start of the program name only.    BUG 2003-12 */
    error = execl(cmd, cmd, p1, p2, p3, p4, NULL);
    printf ("Error %d %s \"%s\", %s, %s, %s, %s, %s\n",
      errno, get_html_text("202"), cmd, hook_type, p1, p2, p3, p4);
    /* if (error == -1) return (-1); */
    exit(127);
  } else {
    wait(&pid);
  }

  return (0);
}

deluser()
{
  send_template( "del_user_confirm.html" );
}

delusergo()
{
 static char forward[200] = "";
 static char forwardto[200] = "";
 FILE *fs;
 int i;
 struct vqpasswd *pw;
     
  if ( AdminType!=DOMAIN_ADMIN ) {
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }

  vdeluser( ActionUser, Domain );

  snprinth (StatusMessage, sizeof(StatusMessage), "%H %s", ActionUser, get_html_text("141"));

  /* Start create forward when delete - 
   * Code added by Eugene Teo 6 June 2000 
   * Modified by Jeff Hedlund (jeff.hedlund@matrixsi.com) 4 June 2003 
   */

  GetValue(TmpCGI,forward, "forward=", sizeof(forward));
  if (strcmp(forward, "on") == 0) {
    GetValue(TmpCGI, forwardto, "forwardto=", sizeof(forwardto));    
    if(adddotqmail_shared(ActionUser, forwardto, -1)!=0) {
       snprintf (StatusMessage, sizeof(StatusMessage), get_html_text("315"), forwardto);
    }
  } 

  call_hooks(HOOK_DELUSER, ActionUser, Domain, forwardto, "");
  show_users(Username, Domain, Mytime);
}

count_users()
{
 struct vqpasswd *pw;

  CurPopAccounts = 0;
  pw = vauth_getall(Domain,1,0);
  while(pw!=NULL){
    ++CurPopAccounts;
    pw = vauth_getall(Domain,0,0);
  }
}

setremotecatchall() 
{
  send_template("setremotecatchall.html");
}

void set_qmaildefault(char *opt)
{
 FILE *fs;

  if ( (fs = fopen(".qmail-default", "w")) == NULL ) {
    printf ("%s %s<br>\n", get_html_text("144"), ".qmail-default");
  } else {
    fprintf(fs,"| %s/bin/vdelivermail '' %s\n", VPOPMAILDIR, opt);
    fclose(fs);
  }
  show_users(Username, Domain, Mytime);
  vclose();
  exit(0);
}

setremotecatchallnow() 
{
  GetValue(TmpCGI,Newu, "newu=", sizeof(Newu));

  if (check_email_addr(Newu) ) {
    snprinth (StatusMessage, sizeof(StatusMessage), "%s %H\n", get_html_text("148"), Newu);
    setremotecatchall();
    exit(0);
  }
  set_qmaildefault (Newu);
}

void bounceall()
{
  set_qmaildefault ("bounce-no-mailbox");
}

void deleteall()
{
  set_qmaildefault ("delete");
}

int get_catchall(void)
{
 int i,j;
 FILE *fs;

  /* Get the default catchall box name */
  if ((fs=fopen(".qmail-default","r")) == NULL) {
    printf ("<tr><td colspan=\"5\">%s %s</td><tr>\n", 
      get_html_text("144"), ".qmail-default");
    vclose();
    exit(0);
  }
  fgets( TmpBuf, sizeof(TmpBuf), fs);
  fclose(fs);

  if (strstr(TmpBuf, " bounce-no-mailbox\n") != NULL) {
    printf ("<b>%s</b>", get_html_text("130"));

  } else if (strstr(TmpBuf, " delete\n") != NULL) {
    printf ("<b>%s</b>", get_html_text("236"));

  } else if ( strstr(TmpBuf, "@") != NULL ) {
    i=strlen(TmpBuf);
    for(;TmpBuf[i]!=' ';--i);
    printh ("<b>%s %H</b>", get_html_text("062"), &TmpBuf[i]);

  } else {
    i = strlen(TmpBuf) - 1;
    for(;TmpBuf[i]!='/';--i);
    for(++i,j=0;TmpBuf[i]!=0;++j,++i) TmpBuf2[j] = TmpBuf[i];
    TmpBuf2[j--] = '\0';

    /* take off newline */
    i = strlen(TmpBuf2); --i; TmpBuf2[i] = 0;/* take off newline */
    printh ("<b>%s %H</b>", get_html_text("062"), TmpBuf2);
  }
  return 0;
}

modusergo()
{
 char crypted[20]; 
 char *tmpstr;
 int i;
 int ret_code;
 int password_updated = 0;
 struct vqpasswd *vpw=NULL;
 static char box[500];
 static char NewBuf[156];
 char *quotaptr;
 char qconvert[11];
 int count;
 FILE *fs;
 int spam_check = 0;

  if (!( AdminType==DOMAIN_ADMIN ||
         (AdminType==USER_ADMIN && strcmp(ActionUser,Username)==0))){
    snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("142"));
    vclose();
    exit(0);
  }

  if (strlen(Password1)>0 && strlen(Password2)>0 ) {
    if ( strcmp( Password1, Password2 ) != 0 ) {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("200"));
      moduser();
      vclose();
      exit(0);
    }
    ret_code = vpasswd( ActionUser, Domain, Password1, USE_POP);
    if ( ret_code != VA_SUCCESS ) {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s (%s)", get_html_text("140"), 
        verror(ret_code));
    } else {
      /* snprinth (StatusMessage, sizeof(StatusMessage), "%s %H@%H.", get_html_text("139"), 
ActionUser, Domain ); */
      strcpy (StatusMessage, get_html_text("139"));
    }
  }

#ifdef MODIFY_QUOTA
  /* strings used: 307 = "Invalid Quota", 308 = "Quota set to unlimited",
   * 309 = "Quota set to %s bytes"
   */
  if (AdminType == DOMAIN_ADMIN) {
    GetValue(TmpCGI, Quota, "quota=", sizeof(Quota));
    vpw = vauth_getpw(ActionUser, Domain); 
    if ((strlen(Quota) == 0) || (strcmp (vpw->pw_shell, Quota) == 0)) {
      /* Blank or no change, do nothing */
    } else if (strncmp(Quota, "NOQUOTA", 2)==0) {
      if (vsetuserquota( ActionUser, Domain, Quota )) {
        snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("307"));  /* invalid quota */
      } else {
        snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("308"));
      }
    } else if (atoi(Quota)) {
      quotaptr = Quota;
      if (quota_to_bytes(qconvert, quotaptr)) {
        snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("307"));
      } else if(strcmp(qconvert, vpw->pw_shell)==0) {
        /* unchanged, do nothing */
      } else if(vsetuserquota( ActionUser, Domain, qconvert )) {
        snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("307"));
      } else { 
        snprintf (StatusMessage, sizeof(StatusMessage), get_html_text("309"), qconvert);
      }
    } else {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s", get_html_text("307"));
    }
  }
#endif

  GetValue(TmpCGI,Gecos, "gecos=", sizeof(Gecos));
  if ( strlen( Gecos ) != 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gecos = Gecos;
    vauth_setpw(vpw, Domain);
  }

  /* check for the V_USERx flags and set accordingly */
  /* James Raftery <james@now.ie>, 12 Dec. 2002 */
  GetValue(TmpCGI,box, "zeroflag=", sizeof(box));
  if ( strcmp(box,"on") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid |= V_USER0;
    vauth_setpw(vpw, Domain);
  } else if ( strcmp(box,"off") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid &= ~V_USER0;
    vauth_setpw(vpw, Domain);
  }
  GetValue(TmpCGI,box, "oneflag=", sizeof(box));
  if ( strcmp(box,"on") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid |= V_USER1;
    vauth_setpw(vpw, Domain);
  } else if ( strcmp(box,"off") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid &= ~V_USER1;
    vauth_setpw(vpw, Domain);
  }
  GetValue(TmpCGI,box, "twoflag=", sizeof(box));
  if ( strcmp(box,"on") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid |= V_USER2;
    vauth_setpw(vpw, Domain);
  } else if ( strcmp(box,"off") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid &= ~V_USER2;
    vauth_setpw(vpw, Domain);
  }
  GetValue(TmpCGI,box, "threeflag=", sizeof(box));
  if ( strcmp(box,"on") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid |= V_USER3;
    vauth_setpw(vpw, Domain);
  } else if ( strcmp(box,"off") == 0 ) {
    vpw = vauth_getpw(ActionUser, Domain); 
    vpw->pw_gid &= ~V_USER3;
    vauth_setpw(vpw, Domain);
  }

  /* get value of the spam filter box */
  GetValue(TmpCGI,box, "spamcheck=", sizeof(box));
  if ( strcmp(box,"on") == 0 ) spam_check = 1;

  /* get the value of the cforward radio button */
  GetValue(TmpCGI,box, "cforward=", sizeof(box));
  
  /* if they want to disable everything */
  if ( strcmp(box,"disable") == 0 ) {

    /* unlink the .qmail file */
    if ( vpw == NULL ) vpw = vauth_getpw(ActionUser, Domain); 
    snprintf(NewBuf,sizeof(NewBuf),"%s/.qmail", vpw->pw_dir);
    unlink(NewBuf);

    /* if the mail is to be checked for spam, rewrite the file with command */
    if( spam_check == 1 ) {
       fs = fopen(NewBuf, "w+");
       fprintf(fs, "%s\n", SPAM_COMMAND);
       fclose(fs);
    }

    /* delete any vacation directory */
    snprintf(NewBuf,sizeof(NewBuf),"%s/vacation", vpw->pw_dir);
    vdelfiles(NewBuf);

  /* if they want to forward */
  } else if (strcmp(box,"forward") == 0 ) {

    /* get the value of the foward */
    GetValue(TmpCGI,box, "nforward=", sizeof(box));

    /* If nothing was entered, error */
    if ( box[0] == 0 ) {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("215"));
      moduser();
      vclose();
      exit(0);

    /* check it for a valid email address
    } else if ( check_email_addr( box ) == 1 )  {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("148"));
      moduser();
    */
    }

    /* everything looks good, open the file */
    if ( vpw == NULL ) {
      vpw = vauth_getpw(ActionUser, Domain); 
    }
    snprintf(NewBuf,sizeof(NewBuf),"%s/.qmail", vpw->pw_dir);

    fs = fopen(NewBuf,"w+");
    tmpstr = strtok(box," ,;\n");

    /* tmpstr points to first non-token */

    count=0;
    while( tmpstr != NULL && count < MAX_FORWARD_PER_USER) {
      if ((*tmpstr != '|') && (*tmpstr != '/')) {
        fprintf(fs,"&%s\n", tmpstr);
        ++count;
      }
      tmpstr = strtok(NULL," ,;\n");
    }

    /* if they want to save a copy */
    GetValue(TmpCGI,box, "fsaved=", sizeof(box));
    if ( strcmp(box,"on") == 0 ) {
      if( spam_check == 1 ) {
        /* if spam check is enabled, that will save the message*/
        fprintf(fs, "%s\n", SPAM_COMMAND);
      } else {
        fprintf(fs,"%s/" MAILDIR "/\n", vpw->pw_dir);
      }
    } 
    fclose(fs);

  /* they want vacation */
  } else if (strcmp(box,"vacation") == 0 ) {

    /* get the subject */
    GetValue(TmpCGI,box, "vsubject=", sizeof(box));

    /* if no subject, error */
    if ( box[0] == 0 ) {
      snprintf (StatusMessage, sizeof(StatusMessage), "%s\n", get_html_text("216"));
      moduser();
      vclose();
      exit(0);
    }
 
    /* make the vacation directory */
    if ( vpw == NULL ) vpw = vauth_getpw(ActionUser, Domain); 
    snprintf(NewBuf,sizeof(NewBuf),"%s/vacation", vpw->pw_dir);
    mkdir(NewBuf, 0750);

    /* open the .qmail file */
    snprintf(NewBuf,sizeof(NewBuf),"%s/.qmail", vpw->pw_dir);
    fs = fopen(NewBuf,"w+");
    fprintf(fs, "| %s/autorespond 86400 3 %s/vacation/message %s/vacation\n",
      AUTORESPOND_PATH, vpw->pw_dir, vpw->pw_dir );

    /* save a copy for the user (if checking for spam, it will keep a copy)*/
    if(spam_check==1)
       fprintf(fs, "%s\n", SPAM_COMMAND);
    else
      fprintf(fs,"%s/" MAILDIR "/\n", vpw->pw_dir);
    fclose(fs);

    /* set up the message file */
    snprintf(NewBuf,sizeof(NewBuf),"%s/vacation/message", vpw->pw_dir);
    GetValue(TmpCGI,Message, "vmessage=",sizeof(Message));

    if ( (fs = fopen(NewBuf, "w")) == NULL ) ack("123", 123);
    fprintf(fs, "From: %s@%s\n", ActionUser,Domain);
    fprintf(fs, "Subject: %s\n\n", box);
    fprintf(fs, "%s", Message);
    fclose(fs);

    /* save the forward for vacation too */
    GetValue(TmpCGI,box,"nforward=", sizeof(box));
    snprintf(NewBuf, sizeof(NewBuf), "%s/.qmail", vpw->pw_dir);
    fs = fopen(NewBuf, "a+");
    tmpstr = strtok(box, " ,;\n");
    count = 0;
    while( tmpstr != NULL && count < 2 ) {
      fprintf(fs, "&%s\n", tmpstr);
      tmpstr = strtok(NULL, " ,;\n");
      ++count;
    }
    fclose(fs);

  /* they want to blackhole */
  /* James Raftery <james@now.ie>, 21 May 2003 */
  } else if (strcmp(box,"blackhole") == 0 ) {

    /* open the .qmail file */
    snprintf(NewBuf,sizeof(NewBuf),"%s/.qmail", vpw->pw_dir);
    fs = fopen(NewBuf,"w");

    /* we use '# delete' since comments are ignored in vdelivermail, 
     * but qmailadmin looks for ' delete' to know that it's a
     * blackhole account. */
    fprintf(fs, "# delete\n");

    fclose(fs);

  } else {
    /* is this an error conditition? none of radios selected? */
    printf("nothing\n");
  }

  call_hooks(HOOK_MODUSER, ActionUser, Domain, Password1, Gecos);
  moduser();
}
