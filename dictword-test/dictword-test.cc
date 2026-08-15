#include <QApplication>
#include <QDebug>
#include <QString>

#include <stdexcept>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>

#pragma region "Shared Code"

// this is an ugly hack, but not as ugly as the previous method with prematurely
// exiting before a segfault happens...
class HtmlCallback : public std::runtime_error {
public:
    HtmlCallback(QString prefix) : std::runtime_error("got html path") {
        this->_prefix = prefix;
    }
    ~HtmlCallback() throw() {};
    QString prefix() const {
        return this->_prefix;
    }
private:
    QString _prefix;
};

#pragma endregion
#pragma region "Application"

// _dwt_main is called by the shim.
static int _dwt_main(int argc, char** argv) {
    // we exit using _Exit, so output doesn't get flushed automatically, and
    // causes printfs to be lost when piping (printf won't flush on newlines
    // when piped), so we'll just disable it entirely.
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    bool useJapanesePath = false;

    int opt;
    while ((opt = getopt(argc, argv, "j")) != -1) {
        switch (opt) {
            case 'j':
                fprintf(stderr, "Setting Japanese mode (useJapanesePath=true)\n");
                useJapanesePath = true;
                break;
        }
    }

    if (optind == argc) {
        fprintf(stderr, "Usage: LD_PRELOAD=%s %s [-j] word_utf8...\n", argv[0], argv[0]);
        fprintf(stderr, "Use -j to enable Japanese mode\n");
        return 2;
    }

    fprintf(stderr, "Initializing QCoreApplication\n");
    QScopedPointer<QCoreApplication> app(new QCoreApplication(argc, argv)); // required for libnickel

    typedef void* DictionaryParser;
    typedef void (*htmlForWord_t)(QString* sret, DictionaryParser _this, QString const& word, bool useJapanesePath);

    fprintf(stderr, "Loading libnickel\n");
    void* libnickel = dlopen("/usr/local/Kobo/libnickel.so.1.0.0", RTLD_LAZY);
    if (!libnickel) {
        fprintf(stderr, "Error: dlopen libnickel: %s\n", dlerror());
        return 1;
    }

    const char* htmlForWordSymbol = "_ZNK16DictionaryParser11htmlForWordERK7QStringb";
    fprintf(stderr, "Loading DictionaryParser::htmlForWord (%s)\n", htmlForWordSymbol);
    htmlForWord_t htmlForWord = (htmlForWord_t) dlsym(libnickel, htmlForWordSymbol);
    if (!htmlForWord) {
        fprintf(stderr, "Error: dlsym %s: %s\n", htmlForWordSymbol, dlerror());
        return 1;
    }

    // TODO: reimplement stdin reading, clean up code

    argv += optind;
    while (--argc >= optind) {
        char *word = *(argv++);
        try {
            QString qword = QString::fromUtf8(word);
            if (qword.isEmpty())
                throw std::runtime_error("empty string not supported"); // libnickel doesn't support this

            fprintf(stderr, "Calling DictionaryParser::htmlForWord\n");
            try {
                // the this pointer isn't used until after getHtml is called and
                // triggers the callback, which is why this works
                QString sretBuf; // just scratch space; its contents don't matter since getHtml throws before this returns normally
                DictionaryParser realThis = calloc(1, 256); // guess at a safe size

                htmlForWord(&sretBuf, realThis, qword, useJapanesePath);

                throw std::runtime_error("callback wasn't called...");
            } catch (HtmlCallback const& cb) {
                printf("{\"%s\", \"%s\"},\n", word, qPrintable(cb.prefix()));
            }
        } catch (std::exception const& ex) {
            printf("// {\"%s\", err(%s)},\n", word, ex.what());
        }
    }

    return 0;
}

#pragma endregion
#pragma region "LD_PRELOAD Library"

// DictionaryParser::getHtml(QString const& path) is called with the path of the
// dictzip and the shard after computing the word prefix in
// DictionaryParser::htmlForWord(QString const& word).
//
// Note that the this pointer isn't used between DictionaryParser::htmlForWord
// and the beginning of DictionaryParser::getHtml, hence why this whole thing
// works and why throw an exception here (the stack will still get unwound
// properly, which is a nice bonus of using this method).
extern "C" void _ZNK16DictionaryParser7getHtmlERK7QString(void* sret, void* _this, QString const& path) {
    fprintf(stderr, "Intercepting getHtml\n");
    throw HtmlCallback(path);
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
