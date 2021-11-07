############################################################################### 
# UniqLogger build configuration

# Uncomment if you plan to use VisualStudio as your IDE and need to generate a project file
#IDE = VS

# If you want to enable network logging uncomment the following line
DEFINES += ENABLE_UNQL_NETLOG

# If you want to enable db logging uncomment the following line
#DEFINES += ENABLE_UNQL_DBLOG

#If you want to enable native Android logging uncomment the following line (Android NDK required)
#DEFINES += ENABLE_UNQL_ANDROIDLOG

# If you want to enable debug statements in the UniqLogger library uncomment the following line
#DEFINES += ENABLE_UNQL_DBG

# explicitly enable c++11 support (needed on some old compliler)
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

