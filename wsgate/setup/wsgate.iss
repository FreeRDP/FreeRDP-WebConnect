;
; $Id: opennx.iss 643 2011-06-23 12:07:58Z felfert $
;
#undef DEBUG

#define APPNAME "FreeRDP-WebConnect"
; Automatically get version from executable resp. dll
#define APPEXE "setupdir\bin\wsgate.exe"
#include "version.iss"

#define APPIDSTR "{93F8582A-FA30-440D-9145-18CE33AA40F7}"
#define APPIDVAL "{" + APPIDSTR

[Setup]
AppName={#=APPNAME}
AppVersion={#=APPFULLVER}
AppVerName={#=APPFULLVERNAME}
AppPublisher=Fritz Elfert
AppPublisherURL=https://github.com/FreeRDP/FreeRDP-WebConnect
AppCopyright=(C) 2012 Fritz Elfert
VersionInfoVersion={#=APPFULLVER}
DefaultDirName={pf}\{#=APPNAME}
DefaultGroupName={#=APPNAME}
#ifdef DEBUG
PrivilegesRequired=none
#endif
DisableStartupPrompt=true
OutputDir=.
OutputBaseFileName={#=SETUPFVNAME}
ShowLanguageDialog=no
MinVersion=0,5.0.2195sp3
AppID={#=APPIDVAL}
UninstallFilesDir={app}\uninstall
Compression=lzma/ultra64
SolidCompression=yes
SetupLogging=yes
WizardImageFile=compiler:wizmodernimage-IS.bmp
WizardSmallImageFile=compiler:wizmodernsmallimage-IS.bmp
; The following breaks in older wine versions, so we
; check the wine version in the invoking script and
; define BADWINE, if we are crossbuilding and have a
; broken wine version.
;#ifndef BADWINE
;SetupIconFile=setupdir\bin\nx.ico
;#endif
;UninstallDisplayIcon={app}\bin\opennx.exe
;LicenseFile=lgpl.rtf

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
uninst_wsgate=Uninstall {#=APPNAME}
doc=API Documentation
binshell=Start commandline in bin directory
fwadd=Adding firewall rules

de.uninst_wsgate=Deinstalliere {#=APPNAME}
de.doc=API Dokumentation
de.binshell=Starte Kommandozeile im bin directory
de.fwadd=Erstelle Firewall-Regeln

cfgssl1=SSL Setup
cfgssl2=Server SSL certificate
cfgssl3=Please specify Parameters for the SSL certificate to be created.
cfgssl4=Common name (server hostname):
cfgssl5=Organization:
cfgssl6=Organizational unit:
cfgssl7=Country:
cfgssl8=State/Province:
cfgssl9=Location:

cfgssl1=SSL Einstellungen
cfgssl2=Server SSL Zertifikat
cfgssl3=Bitte geben Sie die Parameter zum Erstellen eines Server-Zertifikats ein.
cfgssl4=Generischer Name (Server-Hostname):
cfgssl5=Organisation:
cfgssl6=Abteilung:
cfgssl7=Länderkennung:
cfgssl8=Bundesland/Provinz:
cfgssl9=Ort:

[Types]
Name: "compact"; Description: "Compact installation"
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "main"; Description: "{#=APPNAME}"; Types: full compact custom; Flags: fixed;
Name: "apidoc"; Description: "API Documentation"; Types: full;

[Files]
Source: setupdir\bin\*; DestDir: {app}\bin; Flags: recursesubdirs replacesameversion restartreplace uninsrestartdelete
Source: setupdir\etc\*; DestDir: {app}\etc; Flags: recursesubdirs
Source: setupdir\webroot\*; DestDir: {app}\webroot; Flags: recursesubdirs
Source: setupdir\doc\*; DestDir: {app}\doc; Flags: recursesubdirs; Components: apidoc;

[Icons]
Name: "{group}\{cm:doc}"; Filename: "{app}\doc\index.html"; Components: apidoc;
Name: "{group}\{cm:binshell}"; Filename: "{cmd}"; WorkingDir: "{app}\bin"
Name: "{group}\{cm:uninst_wsgate}"; Filename: "{uninstallexe}";

[Run]
Filename: "{sys}\netsh.exe"; Parameters: "firewall add allowedprogram ""{app}\bin\wsgate.exe"" ""{#=APPNAME}"" ENABLE"; StatusMsg: {cm:fwadd}; Flags: runhidden skipifdoesntexist; OnlyBelowVersion: 0,6.0.6000; MinVersion: 0,5.1
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=wsgate_in dir=in action=allow protocol=TCP localport=80,443 program=""{app}\bin\wsgate.exe"" description=""{#=APPNAME}"""; StatusMsg: {cm:fwadd}; Flags: runhidden skipifdoesntexist; MinVersion: 0,6.0.6000
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=wsgate_out dir=out action=allow protocol=TCP program=""{app}\bin\wsgate.exe"" description=""{#=APPNAME}"""; StatusMsg: {cm:fwadd}; Flags: runhidden skipifdoesntexist; MinVersion: 0,6.0.6000

[UninstallRun]
Filename: "{sys}\netsh.exe"; Parameters: "firewall delete allowedprogram ""{app}\bin\wsgate.exe"" ALL"; Flags: runhidden skipifdoesntexist; RunOnceId: fwdelwsgate; OnlyBelowVersion: 1.0,6.0.6000; MinVersion: 0,5.1
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=wsgate_in"; Flags: runhidden skipifdoesntexist; RunOnceId: fwdelIwsgate; MinVersion: 0,6.0.6000
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=wsgate_out"; Flags: runhidden skipifdoesntexist; RunOnceId: fwdelOwsgate; MinVersion: 0,6.0.6000



[Code]
const
    CR = #13#10;

var
    SSLPage: TWizardPage;
    SSLParamEdit: Array [0..5] of TEdit;
    SslDefaults: TStringList;

function b2s2(const b: Boolean):String;
begin
  if b then Result := 'true' else Result := 'false';
end;

function logexec(const cmd, args, dir: String; const ShowCmd: Integer; const Wait: TExecWait; var res: Integer):Boolean;
var b: boolean;
begin
  b := Exec(cmd, args, dir, ShowCmd, Wait, res);
  log('Executing "' + cmd + '" "' + args + '" in "' + dir + '" = ' + b2s2(b) + ' (' + IntToStr(res) + ')');
  Result := b;
end;

procedure CreateSSLPage(AAfterId: Integer);
var
  lbl: Array[0..6] of TNewStaticText;
  i, x, lx: Integer;
begin
  SSLPage := CreateCustomPage(AAfterID, CustomMessage('cfgssl1'), CustomMessage('cfgssl2'));
  x := 0;
  for i := 0 to 6 do begin
    lbl[i] := TNewStaticText.Create(SSLPage);
    with lbl[i] do begin
      Caption := CustomMessage('cfgssl' + IntToStr(3 + i));
      if i = 0 then begin
        AutoSize := False;
        WordWrap := True;
        Width := SSLPage.SurfaceWidth;
      end else begin
        AutoSize := True;
        WordWrap := False;
        Top := lbl[i - 1].Top + lbl[i - 1].Height + ScaleY(12);
        lx := Left + Width + ScaleX(12);
        if lx > x then
          x := lx;
      end;
      Parent := SSLPage.Surface;
    end;
    WizardForm.AdjustLabelHeight(lbl[i]);
  end;
  for i := 0 to 5 do begin
    SSLParamEdit[i] := TEdit.Create(SSLPage);
    with SSLParamEdit[i] do begin
      Top := lbl[i+1].Top + (lbl[i+1].Height / 2) - (Height / 2);
      Left := x;
      Width := SSLPage.SurfaceWidth - x;
      Parent := SSLPage.Surface;
      Text := SslDefaults.Strings[i];
    end;
  end;
end;

procedure InitializeWizard();
begin
    SslDefaults := TStringList.Create();
    SslDefaults.Append('');                { CN }
    SslDefaults.Append('SomeCompany');     { O  }
    SslDefaults.Append('SomeDepartement'); { OU }
    SslDefaults.Append('DE');              { C  }
    SslDefaults.Append('SomeState');       { ST }
    SslDefaults.Append('SomeCity');        { L  }
    CreateSSLPage(wpSelectComponents);
end;

procedure CurStepChanged(step: TSetupStep);
var
    cfg: String;
    subj: String;
    tmp: String;
    res: Integer;

begin
    if (step = ssPostInstall) then begin
        subj := '/CN=' + SSLParamEdit[0].Text + '/O=' + SSLParamEdit[1].Text
            + '/OU='  + SSLParamEdit[2].Text + '/C=' + SSLParamEdit[3].Text
            + '/ST='  + SSLParamEdit[4].Text + '/L=' + SSLParamEdit[5].Text
        logexec(ExpandConstant('{app}\bin\openssl.exe'),
            'req -config openssl.cnf -batch -subj "' + subj +
            '" -newkey rsa:2048 -keyout server.pem -nodes -x509 -days 365 -out tmp.crt',
            ExpandConstant('{app}\etc'), SW_HIDE, ewWaitUntilTerminated, res);
        LoadStringFromFile(ExpandConstant('{app}\etc\tmp.crt'), tmp);
        SaveStringToFile(ExpandConstant('{app}\etc\server.pem'), tmp, true);
        DeleteFile(ExpandConstant('{app}\etc\tmp.crt'))
        cfg := '[global]' + CR + 'debug = true' + CR + 'port = 80' + CR + 'redirect = true' + CR;
        cfg := cfg + '[http]' + CR + 'documentroot = webroot' + CR;
        cfg := cfg + '[ssl]' + CR + 'port = 443' + CR + 'certfile = etc/server.pem' + CR;
        cfg := cfg + '[rdpoverride]' + CR + 'nofullwindowdrag = true' + CR;
        SaveStringToFile(ExpandConstant('{app}\etc\wsgate.ini'), cfg, false);
        logexec(ExpandConstant('{app}\bin\wsgate.exe'), '--install', '', SW_HIDE, ewWaitUntilTerminated, res);
        logexec(ExpandConstant('{app}\bin\wsgate.exe'), '--start', '', SW_HIDE, ewWaitUntilTerminated, res);
    end;
end;

procedure CurUninstallStepChanged(step: TUninstallStep);
var
    res: Integer;

begin
    if (step = usUninstall) then begin
        logexec(ExpandConstant('{app}\bin\wsgate.exe'), '--stop', '', SW_HIDE, ewWaitUntilTerminated, res);
        logexec(ExpandConstant('{app}\bin\wsgate.exe'), '--remove', '', SW_HIDE, ewWaitUntilTerminated, res);
    end;
end;
