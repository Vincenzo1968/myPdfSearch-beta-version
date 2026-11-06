bin_PROGRAMS = mypdfsearch

mypdfsearch_SOURCES = \
    main.c \
    mycontentqueuelist.c mycontentqueuelist.h \
    mycrypt.c mycrypt.h \
    mycrypt_rc4.c mycrypt_rc4.h \
    mydecode.c mydecode.h \
    mydictionaryqueuelist.c mydictionaryqueuelist.h \
    myGenHashTable.c myGenHashTable.h \
    myinitarray.c \
    myInitPredefCMapHT.c myInitPredefCMapHT.h \
    myintqueuelist.c myintqueuelist.h \
    mynumstacklist.c mynumstacklist.h \
    myobjrefqueuelist.c myobjrefqueuelist.h \
    myoctal.c \
    myoctal.h \
    myScopeHashTable.c myScopeHashTable.h \
    mystringqueuelist.c mystringqueuelist.h \
    myTernarySearchTree.c myTernarySearchTree.h \
    mytime.c mytime.h \
    myTree.c myTree.h \
    myUnicodeNFC.c myUnicodeNFC.h \
    parser.c parser.h \
    scanner.c scanner.h \
    vlEncoding.c vlEncoding.h \
    vlRedBlackTrees.c vlRedBlackTrees.h \
    mypdfsearch.h \
    cmap_Adobe-CNS1-7.h \
    cmap_Adobe-GB1-6.h \
    cmap_Adobe-Japan1-7.h \
    cmap_Adobe-Japan2-0.h \
    cmap_Adobe-KR-9.h \
    cmap_Adobe-Korea1-2.h \
    cmap_Adobe-Manga1-0.h \
    cmap_kangxi.h \
    unicode_nfc_data.h

AM_CFLAGS = -Wall -W -pedantic -Wno-overlength-strings

if DEBUG_BUILD
AM_CPPFLAGS = -DMYPDFSEARCH_BUILD_DEBUG=1
else
AM_CPPFLAGS = -DMYPDFSEARCH_BUILD_RELEASE=1
endif

if SHOW_TIME
AM_CPPFLAGS += -DMYPDFSEARCH_SHOW_TIME=1
endif

mypdfsearch_LDADD = -lm

dist_doc_DATA = README.md LICENSE

EXTRA_DIST = \
    LICENSES/README.md \
    LICENSES/Unicode-LICENSE.txt \
    LICENSES/Adobe-CMap-Resources-LICENSE.md
