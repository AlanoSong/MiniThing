@ECHO OFF

SET ROOT_DIR=..\..

.\astyle.exe ^
    %ROOT_DIR%\utils\*.cpp ^
    %ROOT_DIR%\utils\*.h ^
    %ROOT_DIR%\core\*.cpp ^
    %ROOT_DIR%\core\*.h ^
    %ROOT_DIR%\gui\*.cpp ^
    %ROOT_DIR%\gui\*.h ^
    -n ^
    --style=google ^
    --attach-namespaces ^
    --attach-classes ^
    --attach-inlines ^
    --attach-extern-c ^
    --pad-oper ^
    --unpad-paren ^
    --pad-header ^
    --align-pointer=name ^
    --keep-one-line-blocks ^
    --keep-one-line-statements ^
    --break-return-type ^
    --close-templates ^
    --break-after-logical ^
    --break-blocks=all

PAUSE