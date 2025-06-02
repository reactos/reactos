#pragma once
/* Saudi Arabic text is in visual order */

static MUI_ENTRY arSASetupInitPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "الرجاء الانتظار بينما يقوم إعداد ReactOS بتهيئة نفسه",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "ويكتشف أجهزتك...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "الرجاء الانتظار...",
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

static MUI_ENTRY arSALanguagePageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "اختيار اللغة",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  الرجاء اختيار اللغة المستخدمة لعملية التثبيت.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   ثم اضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  ستكون هذه اللغة هي اللغة الافتراضية للنظام النهائي.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة  F3 = إنهاء",
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

static MUI_ENTRY arSAWelcomePageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "أهلاً بك في إعداد ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "يقوم هذا الجزء من الإعداد بنسخ نظام تشغيل ReactOS إلى جهاز الكمبيوتر الخاص بك",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "ويجهز الجزء الثاني من الإعداد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  اضغط ENTER لتثبيت أو ترقية ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  اضغط R لإصلاح تثبيت ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  اضغط L لعرض شروط وأحكام ترخيص ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  اضغط F3 للخروج دون تثبيت ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "لمزيد من المعلومات حول ReactOS، يرجى زيارة:",
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
        "ENTER = متابعة  R = إصلاح  L = ترخيص  F3 = إنهاء",
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

static MUI_ENTRY arSAIntroPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "حالة إصدار ReactOS",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS في مرحلة ألفا، مما يعني أنه ليس كاملاً الميزات",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "وهو قيد التطوير المكثف. يوصى باستخدامه فقط لأغراض",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "التقييم والاختبار وليس كنظام تشغيل للاستخدام اليومي.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "احتفظ بنسخة احتياطية من بياناتك أو اختبر على جهاز كمبيوتر ثانوي إذا حاولت",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "تشغيل ReactOS على أجهزة حقيقية.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  اضغط ENTER لمتابعة إعداد ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  اضغط F3 للخروج دون تثبيت ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   F3 = إنهاء",
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

static MUI_ENTRY arSALicensePageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "الترخيص:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "يتم ترخيص نظام ReactOS بموجب شروط",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "GNU GPL مع أجزاء تحتوي على رمز من تراخيص أخرى متوافقة",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "مثل تراخيص X11 أو BSD و GNU LGPL.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "جميع البرامج التي هي جزء من نظام ReactOS هي",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "لذلك يتم إصدارها بموجب GNU GPL بالإضافة إلى الحفاظ على",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "الترخيص الأصلي.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "يأتي هذا البرنامج بدون ضمان أو قيود على الاستخدام",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "باستثناء القانون المحلي والدولي المعمول به. ترخيص",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "ReactOS يغطي التوزيع لأطراف ثالثة فقط.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "إذا لم تتلق لأي سبب نسخة من",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "رخصة جنو العمومية العامة مع ReactOS يرجى زيارة",
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
        "الضمان:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "هذا برنامج مجاني؛ انظر المصدر لشروط النسخ.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "لا يوجد ضمان؛ ولا حتى للمتاجرة أو",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "الملاءمة لغرض معين",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = عودة",
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

static MUI_ENTRY arSADevicePageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "القائمة أدناه تعرض إعدادات الجهاز الحالية.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "الكمبيوتر:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "الشاشة:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "لوحة المفاتيح:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "تخطيط لوحة المفاتيح:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "قبول:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "قبول إعدادات الجهاز هذه",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "يمكنك تغيير إعدادات الأجهزة بالضغط على مفتاحي UP أو DOWN",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "لتحديد إدخال. ثم اضغط على مفتاح ENTER لتحديد بديل",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "الإعدادات.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "عندما تكون جميع الإعدادات صحيحة، حدد \"قبول إعدادات الجهاز هذه\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "واضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   F3 = إنهاء",
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

static MUI_ENTRY arSARepairPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "إعداد ReactOS في مرحلة تطوير مبكرة. لا يدعم بعد",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "جميع وظائف تطبيق إعداد قابل للاستخدام بالكامل.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "وظائف الإصلاح لم يتم تنفيذها بعد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  اضغط U لتحديث نظام التشغيل.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  اضغط R لوحدة تحكم الاسترداد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  اضغط ESC للعودة إلى الصفحة الرئيسية.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  اضغط ENTER لإعادة تشغيل جهاز الكمبيوتر الخاص بك.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = الصفحة الرئيسية  U = تحديث  R = استرداد  ENTER = إعادة تشغيل",
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

static MUI_ENTRY arSAUpgradePageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "يمكن لإعداد ReactOS ترقية أحد تثبيتات ReactOS المتاحة",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "المدرجة أدناه، أو، إذا كان تثبيت ReactOS تالفًا، يمكن لبرنامج الإعداد",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "محاولة إصلاحه.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "وظائف الإصلاح لم يتم تنفيذها بالكامل بعد.",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  اضغط UP أو DOWN لتحديد تثبيت نظام تشغيل.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  اضغط U لترقية تثبيت نظام التشغيل المحدد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  اضغط ESC للمتابعة بتثبيت جديد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  اضغط F3 للخروج دون تثبيت ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = ترقية   ESC = عدم الترقية   F3 = إنهاء",
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

static MUI_ENTRY arSAComputerPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "تريد تغيير نوع الكمبيوتر المراد تثبيته.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  اضغط مفتاح UP أو DOWN لتحديد نوع الكمبيوتر المطلوب.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   ثم اضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  اضغط مفتاح ESC للعودة إلى الصفحة السابقة دون تغيير",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   نوع الكمبيوتر.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   ESC = إلغاء   F3 = إنهاء",
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

static MUI_ENTRY arSAFlushPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "النظام يتأكد الآن من تخزين جميع البيانات على القرص الخاص بك.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "قد يستغرق هذا دقيقة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "عند الانتهاء، سيتم إعادة تشغيل جهاز الكمبيوتر الخاص بك تلقائياً.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "جاري مسح ذاكرة التخزين المؤقت",
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

static MUI_ENTRY arSAQuitPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS لم يتم تثبيته بالكامل.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "أزل القرص المرن من محرك الأقراص A: و",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "جميع أقراص CD-ROM من محركات الأقراص المضغوطة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "اضغط ENTER لإعادة تشغيل جهاز الكمبيوتر الخاص بك.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "الرجاء الانتظار...",
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

static MUI_ENTRY arSADisplayPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "تريد تغيير نوع الشاشة المراد تثبيتها.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  اضغط مفتاح UP أو DOWN لتحديد نوع الشاشة المطلوب.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   ثم اضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  اضغط مفتاح ESC للعودة إلى الصفحة السابقة دون تغيير",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   نوع الشاشة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   ESC = إلغاء   F3 = إنهاء",
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

static MUI_ENTRY arSASuccessPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "تم تثبيت المكونات الأساسية لـ ReactOS بنجاح.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "أزل القرص المرن من محرك الأقراص A: و",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "جميع أقراص CD-ROM من محرك الأقراص المضغوطة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "اضغط ENTER لإعادة تشغيل جهاز الكمبيوتر الخاص بك.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = إعادة تشغيل الكمبيوتر",
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

static MUI_ENTRY arSASelectPartitionEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "القائمة أدناه تعرض الأقسام الموجودة ومساحة القرص غير المستخدمة",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "للأقسام الجديدة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  اضغط UP أو DOWN لتحديد إدخال في القائمة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  اضغط ENTER لتثبيت ReactOS على القسم المحدد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  اضغط C لإنشاء قسم أساسي/منطقي.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  اضغط E لإنشاء قسم ممتد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  اضغط D لحذف قسم موجود.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "الرجاء الانتظار...",
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

static MUI_ENTRY arSAChangeSystemPartition[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "القسم الحالي للنظام على جهاز الكمبيوتر الخاص بك",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "على قرص النظام",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "يستخدم تنسيقاً غير مدعوم من ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "من أجل تثبيت ReactOS بنجاح، يجب على برنامج الإعداد تغيير",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "قسم النظام الحالي إلى قسم جديد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "قسم النظام المرشح الجديد هو:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  لقبول هذا الاختيار، اضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  لتغيير قسم النظام يدوياً، اضغط ESC للعودة إلى",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   قائمة اختيار القسم، ثم حدد أو أنشئ نظاماً جديداً",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   القسم على قرص النظام.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "في حال وجود أنظمة تشغيل أخرى تعتمد على القسم الأصلي",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "قسم النظام، قد تحتاج إما إلى إعادة تكوينها للقسم الجديد",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "قسم النظام، أو قد تحتاج إلى تغيير قسم النظام مرة أخرى",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "إلى القسم الأصلي بعد الانتهاء من تثبيت ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   ESC = إلغاء",
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

static MUI_ENTRY arSAConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "لقد اخترت حذف قسم النظام.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "يمكن أن تحتوي أقسام النظام على برامج تشخيص، وتكوين أجهزة",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "برامج، وبرامج لبدء تشغيل نظام تشغيل (مثل ReactOS) أو غيرها",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "البرامج المقدمة من قبل الشركة المصنعة للأجهزة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "احذف قسم النظام فقط عندما تكون متأكداً من عدم وجود مثل هذه",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "البرامج على القسم، أو عندما تكون متأكداً من أنك تريد حذفها.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "عند حذف القسم، قد لا تتمكن من تمهيد",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "الكمبيوتر من القرص الصلب حتى تنتهي من إعداد ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  اضغط ENTER لحذف قسم النظام. سيُطلب منك",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   تأكيد حذف القسم مرة أخرى لاحقاً.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  اضغط ESC للعودة إلى الصفحة السابقة. لن يتم",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   حذف القسم.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=متابعة  ESC=إلغاء",
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

static MUI_ENTRY arSAFormatPartitionEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "تنسيق القسم",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "سيقوم الإعداد الآن بتنسيق القسم. اضغط ENTER للمتابعة.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = متابعة   F3 = إنهاء",
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

static MUI_ENTRY arSACheckFSEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "الإعداد يتحقق الآن من القسم المحدد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "الرجاء الانتظار...",
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

static MUI_ENTRY arSAInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "يقوم الإعداد بتثبيت ملفات ReactOS على القسم المحدد. اختر",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "دليلاً حيث تريد تثبيت ReactOS:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "لتغيير الدليل المقترح، اضغط BACKSPACE لحذف",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "الأحرف ثم اكتب الدليل الذي تريد تثبيت ReactOS فيه.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "أن يتم تثبيته.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   F3 = إنهاء",
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

static MUI_ENTRY arSAFileCopyEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "الرجاء الانتظار بينما يقوم إعداد ReactOS بنسخ الملفات إلى مجلد تثبيت ReactOS الخاص بك.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "مجلد التثبيت.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "قد يستغرق هذا عدة دقائق لإكماله.",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 الرجاء الانتظار...    ",
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

static MUI_ENTRY arSABootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "الرجاء تحديد مكان تثبيت برنامج تحميل التمهيد:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "تثبيت برنامج تحميل التمهيد على القرص الصلب (MBR و VBR).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "تثبيت برنامج تحميل التمهيد على القرص الصلب (VBR فقط).",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "تثبيت برنامج تحميل التمهيد على قرص مرن.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "تخطي تثبيت برنامج تحميل التمهيد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   F3 = إنهاء",
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

static MUI_ENTRY arSABootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "الإعداد يقوم بتثبيت برنامج تحميل التمهيد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "جاري تثبيت برنامج تحميل التمهيد على الوسائط، الرجاء الانتظار...",
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

static MUI_ENTRY arSABootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "الإعداد يقوم بتثبيت برنامج تحميل التمهيد.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "الرجاء إدخال قرص مرن منسق في محرك الأقراص A:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "واضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   F3 = إنهاء",
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

static MUI_ENTRY arSAKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "تريد تغيير نوع لوحة المفاتيح المراد تثبيتها.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  اضغط مفتاح UP أو DOWN لتحديد نوع لوحة المفاتيح المطلوب.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   ثم اضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  اضغط مفتاح ESC للعودة إلى الصفحة السابقة دون تغيير",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   نوع لوحة المفاتيح.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   ESC = إلغاء   F3 = إنهاء",
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

static MUI_ENTRY arSALayoutSettingsEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "الرجاء تحديد تخطيط ليتم تثبيته افتراضياً.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  اضغط مفتاح UP أو DOWN لتحديد لوحة المفاتيح المطلوبة",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    التخطيط. ثم اضغط ENTER.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  اضغط مفتاح ESC للعودة إلى الصفحة السابقة دون تغيير",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   تخطيط لوحة المفاتيح.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = متابعة   ESC = إلغاء   F3 = إنهاء",
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

static MUI_ENTRY arSAPrepareCopyEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "الإعداد يجهز جهاز الكمبيوتر الخاص بك لنسخ ملفات ReactOS.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "جاري بناء قائمة نسخ الملفات...",
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

static MUI_ENTRY arSASelectFSEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "حدد نظام ملفات من القائمة أدناه.",
        0
    },
    {
        8,
        19,
        "\x07  اضغط UP أو DOWN لتحديد نظام ملفات.",
        0
    },
    {
        8,
        21,
        "\x07  اضغط ENTER لتنسيق القسم.",
        0
    },
    {
        8,
        23,
        "\x07  اضغط ESC لتحديد قسم آخر.",
        0
    },
    {
        0,
        0,
        "ENTER = متابعة   ESC = إلغاء   F3 = إنهاء",
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

static MUI_ENTRY arSADeletePartitionEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "لقد اخترت حذف القسم",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  اضغط L لحذف القسم.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "تحذير: ستفقد جميع البيانات الموجودة على هذا القسم!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  اضغط ESC للإلغاء.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = حذف القسم   ESC = إلغاء   F3 = إنهاء",
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

static MUI_ENTRY arSARegistryEntries[] =
{
    {
        4,
        3,
        " إعداد ReactOS " KERNEL_VERSION_STR " ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "الإعداد يقوم بتحديث تكوين النظام.",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "جاري إنشاء سجلات الريجستري...",
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

MUI_ERROR arSAErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "نجاح\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS لم يتم تثبيته بالكامل على جهاز الكمبيوتر الخاص بك.\n"
        "إذا خرجت من الإعداد الآن، فستحتاج إلى تشغيل الإعداد مرة أخرى لتثبيت ReactOS.\n"
        "\n"
        "  \x07  اضغط ENTER لمتابعة الإعداد.\n"
        "  \x07  اضغط F3 للخروج من الإعداد.",
        "F3 = إنهاء  ENTER = متابعة"
    },
    {
        // ERROR_NO_BUILD_PATH
        "فشل بناء مسارات التثبيت لدليل تثبيت ReactOS!\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_SOURCE_PATH
        "لا يمكنك حذف القسم الذي يحتوي على مصدر التثبيت!\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_SOURCE_DIR
        "لا يمكنك تثبيت ReactOS داخل دليل مصدر التثبيت!\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_NO_HDD
        "تعذر على الإعداد العثور على قرص صلب.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "تعذر على الإعداد العثور على محرك المصدر الخاص به.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "فشل الإعداد في تحميل الملف TXTSETUP.SIF.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "عثر الإعداد على ملف TXTSETUP.SIF تالف.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "عثر الإعداد على توقيع غير صالح في TXTSETUP.SIF.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "تعذر على الإعداد استرداد معلومات محرك أقراص النظام.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_WRITE_BOOT,
        "فشل الإعداد في تثبيت رمز تمهيد %S على قسم النظام.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "فشل الإعداد في تحميل قائمة أنواع الكمبيوتر.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "فشل الإعداد في تحميل قائمة إعدادات العرض.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "فشل الإعداد في تحميل قائمة أنواع لوحة المفاتيح.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "فشل الإعداد في تحميل قائمة تخطيطات لوحة المفاتيح.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_WARN_PARTITION,
        "عثر الإعداد على أن قرصاً صلباً واحداً على الأقل يحتوي على جدول أقسام غير متوافق\n"
        "لا يمكن التعامل معه بشكل صحيح!\n"
        "\n"
        "إنشاء أو حذف الأقسام يمكن أن يدمر جدول الأقسام.\n"
        "\n"
        "  \x07  اضغط F3 للخروج من الإعداد.\n"
        "  \x07  اضغط ENTER للمتابعة.",
        "F3 = إنهاء  ENTER = متابعة"
    },
    {
        // ERROR_NEW_PARTITION,
        "لا يمكنك إنشاء قسم جديد داخل\n"
        "قسم موجود بالفعل!\n"
        "\n"
        "  * اضغط أي مفتاح للمتابعة.",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "فشل الإعداد في تثبيت رمز تمهيد %S على قسم النظام.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_NO_FLOPPY,
        "لا يوجد قرص في محرك الأقراص A:.",
        "ENTER = متابعة"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "فشل الإعداد في تحديث إعدادات تخطيط لوحة المفاتيح.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "فشل الإعداد في تحديث إعدادات سجل العرض.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_IMPORT_HIVE,
        "فشل الإعداد في استيراد ملف Hive.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_FIND_REGISTRY
        "فشل الإعداد في العثور على ملفات بيانات السجل.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_CREATE_HIVE,
        "فشل الإعداد في إنشاء سجلات الريجستري.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "فشل الإعداد في تهيئة السجل.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "الخزانة لا تحتوي على ملف inf صالح.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_CABINET_MISSING,
        "الخزانة غير موجودة.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "الخزانة لا تحتوي على نص إعداد.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_COPY_QUEUE,
        "فشل الإعداد في فتح قائمة انتظار نسخ الملفات.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_CREATE_DIR,
        "تعذر على الإعداد إنشاء أدلة التثبيت.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "فشل الإعداد في العثور على القسم '%S'\n"
        "في TXTSETUP.SIF.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_CABINET_SECTION,
        "فشل الإعداد في العثور على القسم '%S'\n"
        "في الخزانة.\n",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "تعذر على الإعداد إنشاء دليل التثبيت.",
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_WRITE_PTABLE,
        "فشل الإعداد في كتابة جداول الأقسام.\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "فشل الإعداد في إضافة صفحة الرمز إلى السجل.\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "تعذر على الإعداد تعيين الإعدادات المحلية للنظام.\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "فشل الإعداد في إضافة تخطيطات لوحة المفاتيح إلى السجل.\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_UPDATE_GEOID,
        "تعذر على الإعداد تعيين معرف الجغرافيا.\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "اسم دليل غير صالح.\n"
        "\n"
        "  * اضغط أي مفتاح للمتابعة."
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "القسم المحدد ليس كبيراً بما يكفي لتثبيت ReactOS.\n"
        "يجب أن يكون حجم قسم التثبيت على الأقل %lu ميجابايت.\n"
        "\n"
        "  * اضغط أي مفتاح للمتابعة.",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "لا يمكنك إنشاء قسم أساسي أو ممتد جديد في\n"
        "جدول أقسام هذا القرص لأن جدول الأقسام ممتلئ.\n"
        "\n"
        "  * اضغط أي مفتاح للمتابعة."
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "لا يمكنك إنشاء أكثر من قسم ممتد واحد لكل قرص.\n"
        "\n"
        "  * اضغط أي مفتاح للمتابعة."
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "الإعداد غير قادر على تنسيق القسم:\n"
        " %S\n"
        "\n"
        "ENTER = إعادة تشغيل الكمبيوتر"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE arSAPages[] =
{
    {
        SETUP_INIT_PAGE,
        arSASetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        arSALanguagePageEntries
    },
    {
        WELCOME_PAGE,
        arSAWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        arSAIntroPageEntries
    },
    {
        LICENSE_PAGE,
        arSALicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        arSADevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        arSARepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        arSAUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        arSAComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        arSADisplayPageEntries
    },
    {
        FLUSH_PAGE,
        arSAFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        arSASelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        arSAChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        arSAConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        arSASelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        arSAFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        arSACheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        arSADeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        arSAInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        arSAPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        arSAFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        arSAKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        arSABootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        arSALayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        arSAQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        arSASuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        arSABootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        arSABootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        arSARegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING arSAStrings[] =
{
    {STRING_PLEASEWAIT,
        "   الرجاء الانتظار..."},
        {STRING_INSTALLCREATEPARTITION,
            "   ENTER = تثبيت   C = إنشاء أساسي   E = إنشاء ممتد   F3 = إنهاء"},
            {STRING_INSTALLCREATELOGICAL,
                "   ENTER = تثبيت   C = إنشاء قسم منطقي   F3 = إنهاء"},
                {STRING_INSTALLDELETEPARTITION,
                    "   ENTER = تثبيت   D = حذف القسم   F3 = إنهاء"},
                    {STRING_DELETEPARTITION,
                        "   D = حذف القسم   F3 = إنهاء"},
                        {STRING_PARTITIONSIZE,
                            "حجم القسم الجديد:"},
                            {STRING_CHOOSE_NEW_PARTITION,
                                "لقد اخترت إنشاء قسم أساسي على"},
                                {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
                                    "لقد اخترت إنشاء قسم ممتد على"},
                                    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
                                        "لقد اخترت إنشاء قسم منطقي على"},
                                        {STRING_HDPARTSIZE,
                                            "الرجاء إدخال حجم القسم الجديد بالميجابايت."},
                                            {STRING_CREATEPARTITION,
                                                "   ENTER = إنشاء قسم   ESC = إلغاء   F3 = إنهاء"},
                                                {STRING_NEWPARTITION,
                                                    "الإعداد أنشأ قسماً جديداً على"},
                                                    {STRING_PARTFORMAT,
                                                        "سيتم تنسيق هذا القسم لاحقاً."},
                                                        {STRING_NONFORMATTEDPART,
                                                            "لقد اخترت تثبيت ReactOS على قسم جديد أو غير منسق."},
                                                            {STRING_NONFORMATTEDSYSTEMPART,
                                                                "قسم النظام لم يتم تنسيقه بعد."},
                                                                {STRING_NONFORMATTEDOTHERPART,
                                                                    "القسم الجديد لم يتم تنسيقه بعد."},
                                                                    {STRING_INSTALLONPART,
                                                                        "الإعداد يثبت ReactOS على القسم"},
                                                                        {STRING_CONTINUE,
                                                                            "ENTER = متابعة"},
                                                                            {STRING_QUITCONTINUE,
                                                                                "F3 = إنهاء  ENTER = متابعة"},
                                                                                {STRING_REBOOTCOMPUTER,
                                                                                    "ENTER = إعادة تشغيل الكمبيوتر"},
                                                                                    {STRING_DELETING,
                                                                                        "   جاري حذف الملف: %S"},
                                                                                        {STRING_MOVING,
                                                                                            "   جاري نقل الملف: %S إلى: %S"},
                                                                                            {STRING_RENAMING,
                                                                                                "   جاري إعادة تسمية الملف: %S إلى: %S"},
                                                                                                {STRING_COPYING,
                                                                                                    "   جاري نسخ الملف: %S"},
                                                                                                    {STRING_SETUPCOPYINGFILES,
                                                                                                        "الإعداد ينسخ الملفات..."},
                                                                                                        {STRING_REGHIVEUPDATE,
                                                                                                            "   جاري تحديث سجلات الريجستري..."},
                                                                                                            {STRING_IMPORTFILE,
                                                                                                                "   جاري استيراد %S..."},
                                                                                                                {STRING_DISPLAYSETTINGSUPDATE,
                                                                                                                    "   جاري تحديث إعدادات سجل العرض..."},
                                                                                                                    {STRING_LOCALESETTINGSUPDATE,
                                                                                                                        "   جاري تحديث إعدادات اللغة..."},
                                                                                                                        {STRING_KEYBOARDSETTINGSUPDATE,
                                                                                                                            "   جاري تحديث إعدادات تخطيط لوحة المفاتيح..."},
                                                                                                                            {STRING_CODEPAGEINFOUPDATE,
                                                                                                                                "   جاري إضافة معلومات صفحة الرمز..."},
                                                                                                                                {STRING_DONE,
                                                                                                                                    "   تم..."},
                                                                                                                                    {STRING_REBOOTCOMPUTER2,
                                                                                                                                        "   ENTER = إعادة تشغيل الكمبيوتر"},
                                                                                                                                        {STRING_REBOOTPROGRESSBAR,
                                                                                                                                            " سيتم إعادة تشغيل جهاز الكمبيوتر الخاص بك في %li ثانية (ثوانٍ)... "},
                                                                                                                                            {STRING_CONSOLEFAIL1,
                                                                                                                                                "تعذر فتح وحدة التحكم\r\n\r\n"},
                                                                                                                                                {STRING_CONSOLEFAIL2,
                                                                                                                                                    "السبب الأكثر شيوعاً لذلك هو استخدام لوحة مفاتيح USB\r\n"},
                                                                                                                                                    {STRING_CONSOLEFAIL3,
                                                                                                                                                        "لوحات مفاتيح USB غير مدعومة بالكامل بعد\r\n"},
                                                                                                                                                        {STRING_FORMATTINGPART,
                                                                                                                                                            "الإعداد يقوم بتنسيق القسم..."},
                                                                                                                                                            {STRING_CHECKINGDISK,
                                                                                                                                                                "الإعداد يتحقق من القرص..."},
                                                                                                                                                                {STRING_FORMATDISK1,
                                                                                                                                                                    " تنسيق القسم كنظام ملفات %S (تنسيق سريع) "},
                                                                                                                                                                    {STRING_FORMATDISK2,
                                                                                                                                                                        " تنسيق القسم كنظام ملفات %S "},
                                                                                                                                                                        {STRING_KEEPFORMAT,
                                                                                                                                                                            " الاحتفاظ بنظام الملفات الحالي (لا توجد تغييرات) "},
                                                                                                                                                                            {STRING_HDDISK1,
                                                                                                                                                                                "%s."},
                                                                                                                                                                                {STRING_HDDISK2,
                                                                                                                                                                                    "على %s."},
                                                                                                                                                                                    {STRING_PARTTYPE,
                                                                                                                                                                                        "النوع 0x%02x"},
                                                                                                                                                                                        {STRING_HDDINFO1,
                                                                                                                                                                                            "%I64u %s القرص الصلب %lu (المنفذ=%hu، الناقل=%hu، المعرف=%hu) على %wZ [%s]"},
                                                                                                                                                                                            {STRING_HDDINFO2,
                                                                                                                                                                                                "%I64u %s القرص الصلب %lu (المنفذ=%hu، الناقل=%hu، المعرف=%hu) [%s]"},
                                                                                                                                                                                                {STRING_UNPSPACE,
                                                                                                                                                                                                    "مساحة غير مقسمة"},
                                                                                                                                                                                                    {STRING_MAXSIZE,
                                                                                                                                                                                                        "ميجابايت (الحد الأقصى %lu ميجابايت)"},
                                                                                                                                                                                                        {STRING_EXTENDED_PARTITION,
                                                                                                                                                                                                            "قسم ممتد"},
                                                                                                                                                                                                            {STRING_UNFORMATTED,
                                                                                                                                                                                                                "جديد (غير منسق)"},
                                                                                                                                                                                                                {STRING_FORMATUNUSED,
                                                                                                                                                                                                                    "غير مستخدم"},
                                                                                                                                                                                                                    {STRING_FORMATUNKNOWN,
                                                                                                                                                                                                                        "غير معروف"},
                                                                                                                                                                                                                        {STRING_KB,
                                                                                                                                                                                                                            "ك.ب"},
                                                                                                                                                                                                                            {STRING_MB,
                                                                                                                                                                                                                                "م.ب"},
                                                                                                                                                                                                                                {STRING_GB,
                                                                                                                                                                                                                                    "ج.ب"},
                                                                                                                                                                                                                                    {STRING_ADDKBLAYOUTS,
                                                                                                                                                                                                                                        "جاري إضافة تخطيطات لوحة المفاتيح"},
                                                                                                                                                                                                                                        {0, 0}
};
