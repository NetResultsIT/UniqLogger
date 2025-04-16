TEMPLATE = subdirs

DEFINES += ENABLE_UNQL_DEBUG

SUBDIRS += \
    TestLogPause \
    TestMessagesCompression \
    TestTimeUtilsTicking \
    TestFileWriterRotationScenarios \
    TestDummyDelete
