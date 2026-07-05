cd /d "%~dp0..\.."
start x64\Debug\WinXShell.exe -console -ui -jcfg wxsUI\UI_Shutdown\main.jcfg
pause
start x64\Debug\WinXShell.exe -console -ui -jcfg wxsUI\UI_Shutdown\full.jcfg -blur 5
pause
start x64\Debug\WinXShell.exe -console -ui -jcfg wxsUI\UI_Shutdown\full.jcfg -blur 5 -bk bk.jpg
