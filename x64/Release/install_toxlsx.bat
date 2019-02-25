cd /d %~dp0
rem 注册表键值为路径时需要将\转义才能在.reg文件中生效
set curPath=%~dp0
set curPath=%curPath:\=\\%
set menuName=toxlsx

echo Windows Registry Editor Version 5.00>> tmp.reg
echo.>> tmp.reg

rem 文件夹上右键
echo [HKEY_CLASSES_ROOT\Directory\shell]>> tmp.reg
echo.>> tmp.reg
echo [HKEY_CLASSES_ROOT\Directory\shell\toxlsx]>> tmp.reg
echo @="%menuName%">> tmp.reg
echo.>> tmp.reg
echo [HKEY_CLASSES_ROOT\Directory\shell\toxlsx\command]>> tmp.reg
echo @="\"%curPath%toxlsx.exe\" %%1">> tmp.reg
echo.>> tmp.reg


regedit /s tmp.reg
@del /q tmp.reg