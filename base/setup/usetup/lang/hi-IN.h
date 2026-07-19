#pragma once

static MUI_ENTRY hiINSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "कृपया प्रतीक्षा करें, ReactOS सेटअप स्वयं को तैयार कर रहा है",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "और आपके डिवाइस खोज रहा है...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "कृपया प्रतीक्षा करें...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "भाषा चयन",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  कृपया इंस्टॉलेशन प्रक्रिया के लिए उपयोग की जाने वाली भाषा चुनें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   फिर ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  यह भाषा अंतिम सिस्टम की डिफ़ॉल्ट भाषा होगी।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें  F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS सेटअप में आपका स्वागत है",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "सेटअप का यह भाग ReactOS ऑपरेटिंग सिस्टम को आपके",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "कंप्यूटर पर कॉपी करता है और सेटअप के दूसरे भाग के लिए तैयार करता है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  ReactOS इंस्टॉल या अपग्रेड करने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
     // "\x07  Recovery Console का उपयोग करके ReactOS इंस्टॉलेशन की मरम्मत के लिए R दबाएं।",
        "\x07  ReactOS इंस्टॉलेशन की मरम्मत के लिए R दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  ReactOS की लाइसेंस शर्तें देखने के लिए L दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS इंस्टॉल किए बिना बाहर निकलने के लिए F3 दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "ReactOS के बारे में अधिक जानकारी के लिए कृपया यहाँ जाएँ:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "https://reactos.org/",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें  R = मरम्मत  L = लाइसेंस  F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS संस्करण की स्थिति",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS अभी अल्फा चरण में है, यानी यह अभी पूरी तरह सुविधा-संपन्न नहीं है",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "और अभी व्यापक विकास के अधीन है। इसका उपयोग केवल",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "मूल्यांकन और परीक्षण के लिए करने की सलाह दी जाती है, रोज़मर्रा के उपयोग के लिए नहीं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "यदि आप ReactOS को असली हार्डवेयर पर चलाने की कोशिश करते हैं, तो अपने",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "डेटा का बैकअप लें या किसी दूसरे कंप्यूटर पर परीक्षण करें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  ReactOS सेटअप जारी रखने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS इंस्टॉल किए बिना बाहर निकलने के लिए F3 दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "लाइसेंस:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS सिस्टम को GNU GPL की शर्तों के अंतर्गत लाइसेंस प्राप्त है, जिसमें",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "अन्य संगत लाइसेंसों जैसे X11, BSD और GNU LGPL का कोड",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "भी कुछ हिस्सों में शामिल है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "ReactOS सिस्टम का हिस्सा होने वाला सभी सॉफ़्टवेयर इसलिए",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "GNU GPL के अंतर्गत जारी किया जाता है, और साथ ही",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "मूल लाइसेंस भी बना रहता है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "इस सॉफ़्टवेयर के साथ कोई वारंटी या उपयोग पर प्रतिबंध नहीं है,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "सिवाय लागू स्थानीय और अंतरराष्ट्रीय कानून के। ReactOS का लाइसेंस",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "केवल तृतीय पक्षों को वितरण के मामले में लागू होता है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "यदि किसी कारणवश आपको ReactOS के साथ GNU General Public",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "License की प्रति नहीं मिली है, तो कृपया यहाँ जाएँ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "http://www.gnu.org/licenses/licenses.html",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        22,
        "वारंटी:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "यह एक मुक्त सॉफ़्टवेयर है; कॉपी करने की शर्तों के लिए सोर्स देखें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "इसकी कोई वारंटी नहीं है; न तो व्यापारिक उपयोगिता के लिए और न ही",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "किसी विशेष उद्देश्य के लिए उपयुक्तता के लिए",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = वापस जाएँ",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "नीचे दी गई सूची वर्तमान डिवाइस सेटिंग्स दिखाती है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "कंप्यूटर:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "डिस्प्ले:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "कीबोर्ड:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "कीबोर्ड लेआउट:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "स्वीकार करें:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "इन डिवाइस सेटिंग्स को स्वीकार करें",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "UP या DOWN कुंजी दबाकर आप एक प्रविष्टि चुन",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "सकते हैं और हार्डवेयर सेटिंग्स बदल सकते हैं। फिर वैकल्पिक सेटिंग्स चुनने",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "के लिए ENTER कुंजी दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "जब सभी सेटिंग्स सही हो जाएं, तो \"इन डिवाइस सेटिंग्स को स्वीकार करें\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "चुनें और ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS सेटअप अभी विकास के शुरुआती चरण में है। यह अभी",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "पूरी तरह से उपयोग योग्य सेटअप एप्लिकेशन के सभी फ़ंक्शन समर्थित नहीं करता।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "मरम्मत संबंधी फ़ंक्शन अभी लागू नहीं किए गए हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  OS अपडेट करने के लिए U दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Recovery Console के लिए R दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  मुख्य पृष्ठ पर वापस जाने के लिए ESC दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  अपना कंप्यूटर रीबूट करने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = मुख्य पृष्ठ  U = अपडेट  R = रिकवरी  ENTER = रीबूट",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS सेटअप नीचे सूचीबद्ध उपलब्ध ReactOS इंस्टॉलेशन में से किसी",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "एक को अपग्रेड कर सकता है, या यदि कोई ReactOS इंस्टॉलेशन क्षतिग्रस्त है, तो सेटअप प्रोग्राम",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "उसकी मरम्मत करने की कोशिश कर सकता है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "सभी मरम्मत संबंधी फ़ंक्शन अभी लागू नहीं किए गए हैं।",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  किसी OS इंस्टॉलेशन को चुनने के लिए UP या DOWN दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  चयनित OS इंस्टॉलेशन को अपग्रेड करने के लिए U दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  नई इंस्टॉलेशन के साथ आगे बढ़ने के लिए ESC दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS इंस्टॉल किए बिना बाहर निकलने के लिए F3 दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = अपग्रेड   ESC = अपग्रेड न करें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "आप इंस्टॉल किए जाने वाले कंप्यूटर के प्रकार को बदलना चाहते हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  इच्छित कंप्यूटर प्रकार चुनने के लिए UP या DOWN कुंजी दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   फिर ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  बिना बदलाव किए पिछले पृष्ठ पर वापस जाने के लिए ESC कुंजी दबाएं,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   कंप्यूटर का प्रकार अपरिवर्तित रहेगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   ESC = रद्द करें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "सिस्टम अब यह सुनिश्चित कर रहा है कि सभी डेटा आपकी डिस्क पर संग्रहीत हो गया है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "इसमें कुछ समय लग सकता है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "पूरा होने पर, आपका कंप्यूटर स्वचालित रूप से रीबूट हो जाएगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "कैश फ्लश किया जा रहा है",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS पूरी तरह से इंस्टॉल नहीं हुआ है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "ड्राइव A: से फ्लॉपी डिस्क निकालें और",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "सभी CD-ROM को CD-ड्राइव से निकालें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "अपना कंप्यूटर रीबूट करने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "कृपया प्रतीक्षा करें...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "आप इंस्टॉल किए जाने वाले डिस्प्ले के प्रकार को बदलना चाहते हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  इच्छित डिस्प्ले प्रकार चुनने के लिए UP या DOWN कुंजी दबाएं।",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   फिर ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  बिना बदलाव किए पिछले पृष्ठ पर वापस जाने के लिए ESC कुंजी दबाएं,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   डिस्प्ले का प्रकार अपरिवर्तित रहेगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   ESC = रद्द करें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS के बुनियादी घटक सफलतापूर्वक इंस्टॉल हो गए हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "ड्राइव A: से फ्लॉपी डिस्क निकालें और",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "सभी CD-ROM को CD-ड्राइव से निकालें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "अपना कंप्यूटर रीबूट करने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = कंप्यूटर रीबूट करें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "नीचे दी गई सूची मौजूदा पार्टिशन और नए पार्टिशन के लिए",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "खाली डिस्क स्थान दिखाती है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  सूची की कोई प्रविष्टि चुनने के लिए UP या DOWN दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  चयनित पार्टिशन पर ReactOS इंस्टॉल करने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  प्राइमरी/लॉजिकल पार्टिशन बनाने के लिए C दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  एक्सटेंडेड पार्टिशन बनाने के लिए E दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  मौजूदा पार्टिशन हटाने के लिए D दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "कृपया प्रतीक्षा करें...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINChangeSystemPartition[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "आपके कंप्यूटर का वर्तमान सिस्टम पार्टिशन",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "(सिस्टम डिस्क पर)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "ऐसे फॉर्मेट का उपयोग कर रहा है जो ReactOS द्वारा समर्थित नहीं है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "ReactOS को सफलतापूर्वक इंस्टॉल करने के लिए, सेटअप प्रोग्राम को वर्तमान",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "सिस्टम पार्टिशन को एक नए पार्टिशन में बदलना होगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "नया संभावित सिस्टम पार्टिशन है:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  इस चयन को स्वीकार करने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  सिस्टम पार्टिशन को मैन्युअल रूप से बदलने के लिए, पार्टिशन चयन",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   सूची पर वापस जाने के लिए ESC दबाएं, फिर सिस्टम डिस्क पर एक नया",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   सिस्टम पार्टिशन चुनें या बनाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "यदि अन्य ऑपरेटिंग सिस्टम मूल सिस्टम पार्टिशन पर",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "निर्भर हैं, तो आपको उन्हें नए सिस्टम पार्टिशन के लिए फिर से",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "कॉन्फ़िगर करना पड़ सकता है, या ReactOS की इंस्टॉलेशन पूरी होने के बाद",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "सिस्टम पार्टिशन को वापस मूल स्थिति में बदलना पड़ सकता है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   ESC = रद्द करें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "आपने सिस्टम पार्टिशन हटाने का विकल्प चुना है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "सिस्टम पार्टिशन में डायग्नोस्टिक प्रोग्राम, हार्डवेयर कॉन्फ़िगरेशन",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "प्रोग्राम, ऑपरेटिंग सिस्टम शुरू करने वाले प्रोग्राम (जैसे ReactOS)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "या हार्डवेयर निर्माता द्वारा दिए गए अन्य प्रोग्राम हो सकते हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "सिस्टम पार्टिशन तभी हटाएं जब आपको पूरा यकीन हो कि पार्टिशन पर",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "ऐसे कोई प्रोग्राम नहीं हैं, या आप निश्चित रूप से उन्हें हटाना चाहते हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "पार्टिशन हटाने पर, ReactOS सेटअप पूरा होने तक आप",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "हार्डडिस्क से कंप्यूटर बूट नहीं कर पाएंगे।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  सिस्टम पार्टिशन हटाने के लिए ENTER दबाएं। पार्टिशन को",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   हटाने की पुष्टि बाद में फिर से करने के लिए कहा जाएगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  पिछले पृष्ठ पर वापस जाने के लिए ESC दबाएं। पार्टिशन",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   हटाया नहीं जाएगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=जारी रखें  ESC=रद्द करें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "पार्टिशन फॉर्मेट करें",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "सेटअप अब पार्टिशन को फॉर्मेट करेगा। जारी रखने के लिए ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = जारी रखें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "सेटअप अब चयनित पार्टिशन की जांच कर रहा है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "कृपया प्रतीक्षा करें...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "सेटअप चयनित पार्टिशन पर ReactOS फ़ाइलें इंस्टॉल करता है। वह",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "डायरेक्टरी चुनें जहां आप ReactOS को इंस्टॉल करना चाहते हैं:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "सुझाई गई डायरेक्टरी बदलने के लिए, अक्षर हटाने के लिए BACKSPACE दबाएं",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "और फिर वह डायरेक्टरी टाइप करें जहां आप ReactOS को",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "इंस्टॉल करना चाहते हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "कृपया प्रतीक्षा करें, ReactOS सेटअप आपकी ReactOS",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "इंस्टॉलेशन फ़ोल्डर में फ़ाइलें कॉपी कर रहा है।",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "इसे पूरा होने में कई मिनट लग सकते हैं।",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 कृपया प्रतीक्षा करें...    ",
        TEXT_TYPE_STATUS,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "कृपया चुनें कि सेटअप बूटलोडर कहाँ इंस्टॉल करे:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "हार्डडिस्क पर बूटलोडर इंस्टॉल करें (MBR और VBR)।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "हार्डडिस्क पर बूटलोडर इंस्टॉल करें (केवल VBR)।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "फ्लॉपी डिस्क पर बूटलोडर इंस्टॉल करें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "बूटलोडर इंस्टॉलेशन छोड़ें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINBootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "सेटअप बूटलोडर इंस्टॉल कर रहा है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "मीडिया पर बूटलोडर इंस्टॉल किया जा रहा है, कृपया प्रतीक्षा करें...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "सेटअप बूटलोडर इंस्टॉल कर रहा है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "कृपया ड्राइव A: में एक फॉर्मेटेड फ्लॉपी डिस्क डालें",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "चुनें और ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }

};

static MUI_ENTRY hiINKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "आप इंस्टॉल किए जाने वाले कीबोर्ड के प्रकार को बदलना चाहते हैं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  इच्छित कीबोर्ड प्रकार चुनने के लिए UP या DOWN कुंजी दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   फिर ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  बिना बदलाव किए पिछले पृष्ठ पर वापस जाने के लिए ESC कुंजी दबाएं,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   कीबोर्ड का प्रकार अपरिवर्तित रहेगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   ESC = रद्द करें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "कृपया डिफ़ॉल्ट रूप से इंस्टॉल किया जाने वाला लेआउट चुनें।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  इच्छित कीबोर्ड लेआउट चुनने के लिए UP या DOWN कुंजी",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    दबाएं। फिर ENTER दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  बिना बदलाव किए पिछले पृष्ठ पर वापस जाने के लिए ESC कुंजी दबाएं,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   कीबोर्ड लेआउट अपरिवर्तित रहेगा।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = जारी रखें   ESC = रद्द करें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY hiINPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "सेटअप आपके कंप्यूटर को ReactOS फ़ाइलें कॉपी करने के लिए तैयार कर रहा है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "फ़ाइल कॉपी सूची तैयार की जा रही है...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

static MUI_ENTRY hiINSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "नीचे दी गई सूची से एक फ़ाइल सिस्टम चुनें।",
        0
    },
    {
        8,
        19,
        "\x07  फ़ाइल सिस्टम चुनने के लिए UP या DOWN दबाएं।",
        0
    },
    {
        8,
        21,
        "\x07  पार्टिशन को फॉर्मेट करने के लिए ENTER दबाएं।",
        0
    },
    {
        8,
        23,
        "\x07  कोई अन्य पार्टिशन चुनने के लिए ESC दबाएं।",
        0
    },
    {
        0,
        0,
        "ENTER = जारी रखें   ESC = रद्द करें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "आपने जिस पार्टिशन को हटाने का विकल्प चुना है",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  पार्टिशन हटाने के लिए L दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "चेतावनी: इस पार्टिशन पर मौजूद सभी डेटा नष्ट हो जाएगा!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  रद्द करने के लिए ESC दबाएं।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = पार्टिशन हटाएं   ESC = रद्द करें   F3 = बाहर निकलें",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    }
};

static MUI_ENTRY hiINRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " सेटअप ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "सेटअप सिस्टम कॉन्फ़िगरेशन अपडेट कर रहा है।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "रेजिस्ट्री हाइव बनाए जा रहे हैं...",
        TEXT_TYPE_STATUS | TEXT_PADDING_BIG,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        NULL,
        0
    },

};

MUI_ERROR hiINErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "सफल\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS आपके कंप्यूटर पर पूरी तरह से इंस्टॉल नहीं हुआ है।\n"
        "अभी सेटअप बंद करने पर, ReactOS इंस्टॉल करने के लिए आपको दोबारा\n"
        "सेटअप चलाना होगा।\n"
        "\n"
        "  \x07  सेटअप जारी रखने के लिए ENTER दबाएं।\n"
        "  \x07  सेटअप बंद करने के लिए F3 दबाएं।",
        "F3 = बाहर निकलें  ENTER = जारी रखें"
    },
    {
        // ERROR_NO_BUILD_PATH
        "ReactOS इंस्टॉलेशन डायरेक्टरी के लिए इंस्टॉलेशन पथ बनाने में विफल!\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_SOURCE_PATH
        "आप इंस्टॉलेशन सोर्स वाले पार्टिशन को नहीं हटा सकते!\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_SOURCE_DIR
        "आप इंस्टॉलेशन सोर्स डायरेक्टरी के भीतर ReactOS इंस्टॉल नहीं कर सकते!\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_NO_HDD
        "सेटअप को कोई हार्डडिस्क नहीं मिली।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "सेटअप को अपनी सोर्स ड्राइव नहीं मिली।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "सेटअप TXTSETUP.SIF फ़ाइल लोड करने में विफल रहा।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "सेटअप को एक खराब TXTSETUP.SIF मिला।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "सेटअप को TXTSETUP.SIF में एक अमान्य हस्ताक्षर मिला।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "सेटअप सिस्टम ड्राइव की जानकारी प्राप्त नहीं कर सका।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_WRITE_BOOT,
        "सेटअप सिस्टम पार्टिशन पर %S बूटकोड इंस्टॉल करने में विफल रहा।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "सेटअप कंप्यूटर प्रकार की सूची लोड करने में विफल रहा।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "सेटअप डिस्प्ले सेटिंग्स की सूची लोड करने में विफल रहा।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "सेटअप कीबोर्ड प्रकार की सूची लोड करने में विफल रहा।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "सेटअप कीबोर्ड लेआउट की सूची लोड करने में विफल रहा।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_WARN_PARTITION,
        "सेटअप ने पाया कि कम से कम एक हार्डडिस्क में एक असंगत\n"
        "पार्टिशन टेबल है जिसे ठीक से संभाला नहीं जा सकता!\n"
        "\n"
        "पार्टिशन बनाने या हटाने से पार्टिशन टेबल नष्ट हो सकती है।\n"
        "\n"
        "  \x07  सेटअप बंद करने के लिए F3 दबाएं।\n"
        "  \x07  जारी रखने के लिए ENTER दबाएं।",
        "F3 = बाहर निकलें  ENTER = जारी रखें"
    },
    {
        // ERROR_NEW_PARTITION,
        "आप किसी मौजूदा पार्टिशन के भीतर\n"
        "नया पार्टिशन नहीं बना सकते!\n"
        "\n"
        "  * जारी रखने के लिए कोई भी कुंजी दबाएं।",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "सेटअप सिस्टम पार्टिशन पर %S बूटकोड इंस्टॉल करने में विफल रहा।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_NO_FLOPPY,
        "ड्राइव A: में कोई डिस्क नहीं है।",
        "ENTER = जारी रखें"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "सेटअप कीबोर्ड लेआउट सेटिंग्स अपडेट करने में विफल रहा।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "सेटअप डिस्प्ले रेजिस्ट्री सेटिंग्स अपडेट करने में विफल रहा।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_IMPORT_HIVE,
        "सेटअप एक हाइव फ़ाइल इंपोर्ट करने में विफल रहा।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_FIND_REGISTRY
        "सेटअप को रेजिस्ट्री डेटा फ़ाइलें नहीं मिलीं।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_CREATE_HIVE,
        "सेटअप रेजिस्ट्री हाइव बनाने में विफल रहा।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "सेटअप रेजिस्ट्री शुरू करने में विफल रहा।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "कैबिनेट में कोई मान्य inf फ़ाइल नहीं है।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_CABINET_MISSING,
        "कैबिनेट नहीं मिला।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "कैबिनेट में कोई सेटअप स्क्रिप्ट नहीं है।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_COPY_QUEUE,
        "सेटअप कॉपी फ़ाइल कतार खोलने में विफल रहा।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_CREATE_DIR,
        "सेटअप इंस्टॉलेशन डायरेक्टरी नहीं बना सका।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "सेटअप को '%S' सेक्शन\n"
        "TXTSETUP.SIF में नहीं मिला।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_CABINET_SECTION,
        "सेटअप को '%S' सेक्शन\n"
        "कैबिनेट में नहीं मिला।\n",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "सेटअप इंस्टॉलेशन डायरेक्टरी नहीं बना सका।",
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_WRITE_PTABLE,
        "सेटअप पार्टिशन टेबल लिखने में विफल रहा।\n"
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "सेटअप रेजिस्ट्री में कोडपेज जोड़ने में विफल रहा।\n"
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "सेटअप सिस्टम लोकेल सेट नहीं कर सका।\n"
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "सेटअप रेजिस्ट्री में कीबोर्ड लेआउट जोड़ने में विफल रहा।\n"
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_UPDATE_GEOID,
        "सेटअप geo id सेट नहीं कर सका।\n"
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "अमान्य डायरेक्टरी नाम।\n"
        "\n"
        "  * जारी रखने के लिए कोई भी कुंजी दबाएं।"
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "चयनित पार्टिशन ReactOS इंस्टॉल करने के लिए पर्याप्त बड़ा नहीं है।\n"
        "इंस्टॉल पार्टिशन का आकार कम से कम %lu MB होना चाहिए।\n"
        "\n"
        "  * जारी रखने के लिए कोई भी कुंजी दबाएं।",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "आप इस डिस्क की पार्टिशन टेबल में नया प्राइमरी या एक्सटेंडेड\n"
        "पार्टिशन नहीं बना सकते, क्योंकि पार्टिशन टेबल भर गई है।\n"
        "\n"
        "  * जारी रखने के लिए कोई भी कुंजी दबाएं।"
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "आप प्रति डिस्क एक से अधिक एक्सटेंडेड पार्टिशन नहीं बना सकते।\n"
        "\n"
        "  * जारी रखने के लिए कोई भी कुंजी दबाएं।"
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "सेटअप पार्टिशन को फॉर्मेट करने में असमर्थ है:\n"
        " %S\n"
        "\n"
        "ENTER = कंप्यूटर रीबूट करें"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE hiINPages[] =
{
    {
        SETUP_INIT_PAGE,
        hiINSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        hiINLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        hiINWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        hiINIntroPageEntries
    },
    {
        LICENSE_PAGE,
        hiINLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        hiINDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        hiINRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        hiINUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        hiINComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        hiINDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        hiINFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        hiINSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        hiINChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        hiINConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        hiINSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        hiINFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        hiINCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        hiINDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        hiINInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        hiINPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        hiINFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        hiINKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        hiINBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        hiINLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        hiINQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        hiINSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        hiINBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        hiINBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        hiINRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING hiINStrings[] =
{
    {STRING_PLEASEWAIT,
     "   कृपया प्रतीक्षा करें..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = इंस्टॉल   C = प्राइमरी बनाएं   E = एक्सटेंडेड बनाएं   F3 = बाहर निकलें"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = इंस्टॉल   C = लॉजिकल पार्टिशन बनाएं   F3 = बाहर निकलें"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = इंस्टॉल   D = पार्टिशन हटाएं   F3 = बाहर निकलें"},
    {STRING_DELETEPARTITION,
     "   D = पार्टिशन हटाएं   F3 = बाहर निकलें"},
    {STRING_PARTITIONSIZE,
     "नए पार्टिशन का आकार:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "आपने जिस पर प्राइमरी पार्टिशन बनाने का विकल्प चुना है"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "आपने जिस पर एक्सटेंडेड पार्टिशन बनाने का विकल्प चुना है"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "आपने जिस पर लॉजिकल पार्टिशन बनाने का विकल्प चुना है"},
    {STRING_HDPARTSIZE,
    "कृपया नए पार्टिशन का आकार मेगाबाइट में दर्ज करें।"},
    {STRING_CREATEPARTITION,
     "   ENTER = पार्टिशन बनाएं   ESC = रद्द करें   F3 = बाहर निकलें"},
    {STRING_NEWPARTITION,
    "सेटअप ने एक नया पार्टिशन बनाया है"},
    {STRING_PARTFORMAT,
    "यह पार्टिशन अब फॉर्मेट किया जाएगा।"},
    {STRING_NONFORMATTEDPART,
    "आपने ReactOS को एक नए या अनफॉर्मेटेड पार्टिशन पर इंस्टॉल करने का विकल्प चुना है।"},
    {STRING_NONFORMATTEDSYSTEMPART,
    "सिस्टम पार्टिशन अभी फॉर्मेट नहीं हुआ है।"},
    {STRING_NONFORMATTEDOTHERPART,
    "नया पार्टिशन अभी फॉर्मेट नहीं हुआ है।"},
    {STRING_INSTALLONPART,
    "सेटअप जिस पार्टिशन पर ReactOS इंस्टॉल करेगा"},
    {STRING_CONTINUE,
    "ENTER = जारी रखें"},
    {STRING_QUITCONTINUE,
    "F3 = बाहर निकलें  ENTER = जारी रखें"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = कंप्यूटर रीबूट करें"},
    {STRING_DELETING,
     "   फ़ाइल हटाई जा रही है: %S"},
    {STRING_MOVING,
     "   फ़ाइल ले जाई जा रही है: %S \xe2\x86\x92 %S"},
    {STRING_RENAMING,
     "   नाम बदला जा रहा है: %S \xe2\x86\x92 %S"},
    {STRING_COPYING,
     "   फ़ाइल कॉपी की जा रही है: %S"},
    {STRING_SETUPCOPYINGFILES,
     "सेटअप फ़ाइलें कॉपी कर रहा है..."},
    {STRING_REGHIVEUPDATE,
    "   रेजिस्ट्री हाइव अपडेट किए जा रहे हैं..."},
    {STRING_IMPORTFILE,
    "   %S इंपोर्ट किया जा रहा है..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   डिस्प्ले रेजिस्ट्री सेटिंग्स अपडेट की जा रही हैं..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   लोकेल सेटिंग्स अपडेट की जा रही हैं..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   कीबोर्ड लेआउट सेटिंग्स अपडेट की जा रही हैं..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   कोडपेज जानकारी जोड़ी जा रही है..."},
    {STRING_DONE,
    "   पूर्ण..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = कंप्यूटर रीबूट करें"},
    {STRING_REBOOTPROGRESSBAR,
    " आपका कंप्यूटर %li सेकंड में रीबूट होगा... "},
    {STRING_CONSOLEFAIL1,
    "कंसोल खोलने में असमर्थ\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "इसका सबसे सामान्य कारण USB कीबोर्ड का उपयोग करना है\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB कीबोर्ड अभी पूरी तरह समर्थित नहीं हैं\r\n"},
    {STRING_FORMATTINGPART,
    "सेटअप पार्टिशन को फॉर्मेट कर रहा है..."},
    {STRING_CHECKINGDISK,
    "सेटअप डिस्क की जांच कर रहा है..."},
    {STRING_FORMATDISK1,
    " पार्टिशन को %S फ़ाइल सिस्टम के रूप में फॉर्मेट करें (क्विक फॉर्मेट) "},
    {STRING_FORMATDISK2,
    " पार्टिशन को %S फ़ाइल सिस्टम के रूप में फॉर्मेट करें "},
    {STRING_KEEPFORMAT,
    " वर्तमान फ़ाइल सिस्टम रखें (कोई बदलाव नहीं) "},
    {STRING_HDDISK1,
    "%s।"},
    {STRING_HDDISK2,
    "%s पर।"},
    {STRING_PARTTYPE,
    "प्रकार 0x%02x"},
    {STRING_HDDINFO1,
    // "हार्डडिस्क %lu (%I64u %s), पोर्ट=%hu, बस=%hu, आईडी=%hu (%wZ) [%s]"
    "%I64u %s हार्डडिस्क %lu (पोर्ट=%hu, बस=%hu, आईडी=%hu) %wZ पर [%s]"},
    {STRING_HDDINFO2,
    // "हार्डडिस्क %lu (%I64u %s), पोर्ट=%hu, बस=%hu, आईडी=%hu [%s]"
    "%I64u %s हार्डडिस्क %lu (पोर्ट=%hu, बस=%hu, आईडी=%hu) [%s]"},
    {STRING_UNPSPACE,
    "अविभाजित स्थान"},
    {STRING_MAXSIZE,
    "MB (अधिकतम %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "एक्सटेंडेड पार्टिशन"},
    {STRING_UNFORMATTED,
    "नया (अनफॉर्मेटेड)"},
    {STRING_FORMATUNUSED,
    "अप्रयुक्त"},
    {STRING_FORMATUNKNOWN,
    "अज्ञात"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "कीबोर्ड लेआउट जोड़े जा रहे हैं"},
    {0, 0}
};
