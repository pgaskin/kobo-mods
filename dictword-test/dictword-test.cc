#include <QApplication>
#include <QDebug>
#include <QString>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

#pragma region "Application"

// htmlForWord is a dlsym wrapper for DictionaryParser11htmlForWord(QString
// const& word).
//
// Note that this function doesn't need the this pointer to be set until after
// the first call to DictionaryParser::getHtml, hence why this works.
//
// A QCoreApplication (or subclass) must be initialized before calling this
// function. It will not be used for anything, but it is needed by libnickel.
static void* htmlForWord(QString const& word) {
    typedef void* DictionaryParser;
    typedef void* (*htmlForWord_t)(DictionaryParser, QString const&);

    fprintf(stderr, "Loading libnickel\n");
    void* libnickel = dlopen("/usr/local/Kobo/libnickel.so.1.0.0", RTLD_LAZY);
    if (!libnickel) {
        fprintf(stderr, "Error loading libnickel: %s\n", dlerror());
        _Exit(1);
        __builtin_unreachable();
    }

    fprintf(stderr, "Loading DictionaryParser::htmlForWord\n");
    htmlForWord_t fn = (htmlForWord_t) dlsym(libnickel, "_ZN16DictionaryParser11htmlForWordERK7QString");
    if (!fn) {
        fprintf(stderr, "Error loading DictionaryParser::htmlForWord: %s\n", dlerror());
        _Exit(1);
        __builtin_unreachable();
    }

    fprintf(stderr, "Calling DictionaryParser::htmlForWord\n");
    return fn(NULL, word);
}

// _dwt_main is called by the shim.
static int _dwt_main(int argc, char** argv) {
    fprintf(stderr, "Initializing QCoreApplication\n");
    QScopedPointer<QCoreApplication> app(new QCoreApplication(argc, argv));

    if (argc != 2) {
        fprintf(stderr, "Usage: LD_PRELOAD=%s %s <word_utf8>\n", argv[0], argv[0]);
        return 2;
    }

    QString word = QString::fromUtf8(argv[1]);
    htmlForWord(word);

    __builtin_unreachable(); // the getHtml wrapper should exit
}

#pragma endregion
#pragma region "LD_PRELOAD Library"

// DictionaryParser::getHtml(QString const& path) is called with the path of the
// dictzip and the shard after computing the word prefix in
// DictionaryParser::htmlForWord(QString const& word).
//
// Note that the this pointer isn't used between DictionaryParser::htmlForWord
// and the beginning of DictionaryParser::getHtml, hence why this whole thing
// works and why we exit here (we could also patch DictionaryParser::getHtml
// to return after calling this so we can process more than one word at a time,
// but it means this trick won't safely work standalone without interfering with
// nickel).
extern "C" void* _ZN16DictionaryParser7getHtmlERK7QString(void* _this, QString const& path) {
    fprintf(stderr, "Intercepting getHtml\n");

    printf("%s\n", qPrintable(path));
    _Exit(0); // exit to prevent finishing the rest of htmlForWord and segfaulting
    __builtin_unreachable();
}

#pragma endregion
#pragma region "Shims"

// interp sets the interpreter path so this can be called as a executable as
// well as loaded as a library.
__attribute__((unused)) static const char interp[] __attribute__((section(".interp"))) = "/lib/ld-linux.so.3"; // arm-linux-gnueabihf

// allow accessing the args from another func.
int _dwt_argc = 0;
char** _dwt_argv = NULL;

// _dwt_init stores the arguments for _main.
static int _dwt_init(int iargc, char** iargv, char** ienv) {
    fprintf(stderr, "Storing arguments\n");
    _dwt_argc = iargc;
    _dwt_argv = (char**) calloc(_dwt_argc, sizeof(char*));
    for (int i = 0; i < _dwt_argc; i++)
        _dwt_argv[i] = strdup(iargv[i]);
    return 0;
}

// init is called when the binary is loaded.
__attribute__((unused)) static void* init __attribute__((section(".init_array"))) = (void*) &_dwt_init;

// _main_shim is used as the entry point when running as an application.
extern "C" void _dwt_main_shim() {
    if (!_dwt_argc || !_dwt_argv)
        fprintf(stderr, "Warning: Arguments which should have been stored in init aren't there...\n");
    _Exit(_dwt_main(_dwt_argc, _dwt_argv));
    __builtin_unreachable();
}

#pragma endregion
