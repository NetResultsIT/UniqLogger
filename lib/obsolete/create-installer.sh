#!/bin/bash
#

set -e

VENDOR_NAME="netresults"
PACKAGE_NAME="uniqlogger"

######################################
# Create base dir struct
rm -rf installer
mkdir installer
mkdir installer/packages
mkdir installer/config

# Create config.xml template file
touch installer/config/config.xml
cat << '_EOF_' > installer/config/config.xml
<?xml version="1.0" encoding="UTF-8"?>
 <Installer>
     <Name>YourAppNameHere</Name>
     <Version>1.2.3</Version>
     <Title>Your application Installer</Title>
     <Publisher>NetResults Srl</Publisher>
     <StartMenuDir>Super App</StartMenuDir>
     <TargetDir>@rootDir@InstallationDirectory</TargetDir>
 </Installer>
_EOF_

mkdir installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}
mkdir installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/data
mkdir installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/meta

# create package.xml template file
touch installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/meta/package.xml
cat << '_EOF_' > installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/meta/package.xml
<?xml version="1.0" encoding="UTF-8"?>
 <Package>
     <DisplayName>The root component</DisplayName>
     <Description>Install this example.</Description>
     <Version>0.1.0-1</Version>
     <ReleaseDate>2010-09-21</ReleaseDate>
     <Name>com.netresults.uniqlogger</Name>
     <Licenses>
         <License name="Beer Public License Agreement" file="license.txt" />
     </Licenses>
     <Translations>
         <Translation>de_de.qm</Translation>
     </Translations>
     <Default>script</Default>
     <Script>installscript.qs</Script>
 </Package>
_EOF_

# Create license template file
touch installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/meta/license.txt
cat << '_EOF_' > installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/meta/license.txt
This is the NetResults Srl EULA
blablabla blablabla
blablabla blablabla
_EOF_

# create instal script template for components
touch installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/meta/installscript.qs
cat << '_EOF_' > installer/packages/com.${VENDOR_NAME}.${PACKAGE_NAME}/meta/installscript.qs
function Component()
{
   // constructor
}

Component.prototype.isDefault = function()
{
   // select the component by default
   return true;
}

Component.prototype.createOperations = function()
{
   try {
    // call the base create operations function
    component.createOperations();
   } catch (e) {
     print(e);
   }
}
_EOF_




