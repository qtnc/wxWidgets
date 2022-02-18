#!/usr/bin/env python

# Run this script from top-level wxWidgets directory to update the contents of
# include/wx/intl.h and src/common/intl.cpp using information from langtabl.txt
#
# Warning: error detection and reporting here is rudimentary, check if the
# files were updated correctly with "git diff" before committing them!

import os
import string
import sys

def ReadScriptTable():
    scripttable = []
    try:
        f = open('misc/languages/scripttabl.txt')
    except:
        print("Did you run the script from top-level wxWidgets directory?")
        raise

    for i in f.readlines():
        ispl = i.split()
        scripttable.append((ispl[0], ispl[1]))
    f.close()
    return scripttable


def ReadTable():
    table = []
    try:
        f = open('misc/languages/langtabl.txt')
    except:
        print("Did you run the script from top-level wxWidgets directory?")
        raise

    for i in f.readlines():
        ispl = i.split()
        table.append((ispl[0], ispl[1], ispl[2], ispl[3], ispl[4], ispl[5], ispl[6], string.join(ispl[7:])))
    f.close()
    return table


def WriteEnum(f, table, scripttable):
   f.write("""
/**
    The languages supported by wxLocale.

    This enum is generated by misc/languages/genlang.py
    When making changes, please put them into misc/languages/langtabl.txt
*/
enum wxLanguage
{
    /// User's default/preferred language as got from OS.
    wxLANGUAGE_DEFAULT,

    /// Unknown language, returned if wxLocale::GetSystemLanguage fails.
    wxLANGUAGE_UNKNOWN,

""");
   knownLangs = []
   for i in table:
       if i[0] not in knownLangs:
          f.write('    %s,\n' % i[0])
          knownLangs.append(i[0])
   f.write("""
    /// For custom, user-defined languages.
    wxLANGUAGE_USER_DEFINED,

    /// Synonyms.
    wxLANGUAGE_AZERI = wxLANGUAGE_AZERBAIJANI,
    wxLANGUAGE_AZERI_CYRILLIC = wxLANGUAGE_AZERBAIJANI_CYRILLIC,
    wxLANGUAGE_AZERI_LATIN = wxLANGUAGE_AZERBAIJANI_LATIN,
    wxLANGUAGE_BENGALI = wxLANGUAGE_BANGLA,
    wxLANGUAGE_BENGALI_BANGLADESH = wxLANGUAGE_BANGLA_BANGLADESH,
    wxLANGUAGE_BENGALI_INDIA = wxLANGUAGE_BANGLA_INDIA,
    wxLANGUAGE_BHUTANI = wxLANGUAGE_DZONGKHA,
    wxLANGUAGE_CHINESE_SIMPLIFIED = wxLANGUAGE_CHINESE_CHINA,
    wxLANGUAGE_CHINESE_TRADITIONAL = wxLANGUAGE_CHINESE_TAIWAN,
    wxLANGUAGE_CHINESE_MACAU = wxLANGUAGE_CHINESE_MACAO,
    wxLANGUAGE_KERNEWEK = wxLANGUAGE_CORNISH,
    wxLANGUAGE_MALAY_BRUNEI_DARUSSALAM = wxLANGUAGE_MALAY_BRUNEI,
    wxLANGUAGE_ORIYA = wxLANGUAGE_ODIA,
    wxLANGUAGE_ORIYA_INDIA = wxLANGUAGE_ODIA_INDIA,
    wxLANGUAGE_SPANISH_MODERN = wxLANGUAGE_SPANISH,

    /// Obsolete synonym.
    wxLANGUAGE_CAMBODIAN = wxLANGUAGE_KHMER
};

""")


def WriteTable(f, table, scripttable):
   sctable = ''
   for i in scripttable:
       scname = '"%s"' % i[0]
       scalias = '"%s"' % i[1]
       sctable += '    { %s, %s },\n' % (scname, scalias)

   lngtable = ''

   for i in table:
       ibcp47 = '"%s"' % i[1]
       ican = '"%s"' % i[2]
       if ican == '"-"': ican = '""'
       icanbase = '"%s"' % i[3]
       if icanbase == '"-"': icanbase = '""'
       ilang = i[4]
       if ilang == '-': ilang = '0'
       isublang = i[5]
       if isublang == '-': isublang = '0'
       if (i[6] == "LTR") :
           ilayout = "wxLayout_LeftToRight"
       elif (i[6] == "RTL"):
           ilayout = "wxLayout_RightToLeft"
       else:
           print "ERROR: Invalid value for the layout direction";
       lngtable += '    { %-60s %-17s, %-28s, %-15s, %-4s, %-4s, %s, %s },\n' % \
                     ((i[0]+','), ibcp47, ican, icanbase, ilang, isublang, ilayout, i[7])

   f.write("""
// The following data tables are generated by misc/languages/genlang.py
// When making changes, please put them into misc/languages/langtabl.txt

#if !defined(__WIN32__)

#define SETWINLANG(info,lang,sublang)

#else

#define SETWINLANG(info,lang,sublang) \\
    info.WinLang = tabLangData[j].lang; \\
    info.WinSublang = tabLangData[j].sublang;

#endif // __WIN32__

// Data table for known languages
static const struct langData_t
{
    int   wxlang;
    const char* bcp47tag;
    const char* canonical;
    const char* canonicalref;
    wxUint32 winlang;
    wxUint32 winsublang;
    wxLayoutDirection layout;
    const char* desc;
    const char* descnative;
}
tabLangData[] =
{
%s
    { 0, NULL, NULL, NULL, 0, 0, wxLayout_Default, NULL, NULL }
};

// Data table for known language scripts
static const struct scriptData_t
{
    const char* scname;
    const char* scalias;
}
tabScriptData[] =
{
%s
    { NULL, NULL }
};

void wxUILocale::InitLanguagesDB()
{
    wxLanguageInfo info;
    int j;

    // Known languages
    for (j = 0; tabLangData[j].wxlang != 0; ++j)
    {
      info.Language = tabLangData[j].wxlang;
      info.LocaleTag = tabLangData[j].bcp47tag;
      info.CanonicalName = tabLangData[j].canonical;
      info.CanonicalRef = tabLangData[j].canonicalref;
      info.LayoutDirection = tabLangData[j].layout;
      info.Description = wxString::FromUTF8(tabLangData[j].desc);
      info.DescriptionNative = wxString::FromUTF8(tabLangData[j].descnative);
      SETWINLANG(info, winlang, winsublang)
      AddLanguage(info);
    }

    // Known language scripts
    for (j = 0; tabScriptData[j].scname; ++j)
    {
      gs_scmap_name2alias[tabScriptData[j].scname] = tabScriptData[j].scalias;
      gs_scmap_alias2name[tabScriptData[j].scalias] = tabScriptData[j].scname;
    }
}

""" % (lngtable,sctable))


def ReplaceGeneratedPartOfFile(fname, func):
    """
        Replaces the part of file marked with the special comments with the
        output of func.

        fname is the name of the input file and func must be a function taking
        a file and language table on input and writing the appropriate chunk to
        this file, e.g. WriteEnum or WriteTable.
    """
    fin = open(fname, 'rt')
    fnameNew = fname + '.new'
    fout = open(fnameNew, 'wt')
    betweenBeginAndEnd = 0
    afterEnd = 0
    for l in fin.readlines():
        if l == '// --- --- --- generated code begins here --- --- ---\n':
            if betweenBeginAndEnd or afterEnd:
                print 'Unexpected starting comment.'
            betweenBeginAndEnd = 1
            fout.write(l)
            func(fout, table, scripttable)
        elif l == '// --- --- --- generated code ends here --- --- ---\n':
            if not betweenBeginAndEnd:
                print 'End comment found before the starting one?'
                break

            betweenBeginAndEnd = 0
            afterEnd = 1

        if not betweenBeginAndEnd:
            fout.write(l)

    if not afterEnd:
        print 'Failed to process %s.' % fname
        os.remove(fnameNew)
        sys.exit(1)

    fout.close()
    fin.close()
    os.remove(fname)
    os.rename(fnameNew, fname)

table = ReadTable()
scripttable = ReadScriptTable()
ReplaceGeneratedPartOfFile('include/wx/language.h', WriteEnum)
ReplaceGeneratedPartOfFile('interface/wx/language.h', WriteEnum)
ReplaceGeneratedPartOfFile('src/common/languageinfo.cpp', WriteTable)
