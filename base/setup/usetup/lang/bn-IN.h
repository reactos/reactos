#pragma once

static MUI_ENTRY bnINSetupInitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        20,
        "অনুগ্রহ করে অপেক্ষা করুন, ReactOS সেটআপ নিজেকে প্রস্তুত করছে",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        21,
        "এবং আপনার ডিভাইসগুলো খুঁজে দেখছে...",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "অনুগ্রহ করে অপেক্ষা করুন...",
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

static MUI_ENTRY bnINLanguagePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ভাষা নির্বাচন",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  ইনস্টলেশন প্রক্রিয়ায় ব্যবহারের জন্য একটি ভাষা নির্বাচন করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   তারপর ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  এই ভাষাটি শেষ সিস্টেমের ডিফল্ট ভাষা হিসেবে ব্যবহৃত হবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান  F3 = প্রস্থান",
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

static MUI_ENTRY bnINWelcomePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS সেটআপে স্বাগতম",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "সেটআপের এই অংশটি ReactOS অপারেটিং সিস্টেমটি আপনার",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "কম্পিউটারে কপি করে এবং সেটআপের দ্বিতীয় অংশের জন্য প্রস্তুত করে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  ReactOS ইনস্টল বা আপগ্রেড করতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
     // "\x07  Recovery Console ব্যবহার করে ReactOS ইনস্টলেশন মেরামত করতে R টিপুন।",
        "\x07  ReactOS ইনস্টলেশন মেরামত করতে R টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  ReactOS-এর লাইসেন্সের শর্তাবলী দেখতে L টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS ইনস্টল না করে প্রস্থান করতে F3 টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "ReactOS সম্পর্কে আরও তথ্যের জন্য দেখুন:",
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
        "ENTER = এগিয়ে যান  R = মেরামত  L = লাইসেন্স  F3 = প্রস্থান",
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

static MUI_ENTRY bnINIntroPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS সংস্করণের অবস্থা",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "ReactOS বর্তমানে আলফা পর্যায়ে রয়েছে, অর্থাৎ এটি সম্পূর্ণ ফিচারযুক্ত নয়",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "এবং এখনও ব্যাপক ডেভেলপমেন্টের মধ্যে রয়েছে। এটি কেবল মূল্যায়ন ও",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "পরীক্ষার উদ্দেশ্যে ব্যবহার করার পরামর্শ দেওয়া হয়, নিয়মিত ব্যবহারের OS হিসেবে নয়।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "আপনি যদি রিয়েল হার্ডওয়্যারে ReactOS চালানোর চেষ্টা করেন, তাহলে",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "আপনার ডেটা ব্যাকআপ নিন বা দ্বিতীয় কোনো কম্পিউটারে পরীক্ষা করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  ReactOS সেটআপ চালিয়ে যেতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS ইনস্টল না করে প্রস্থান করতে F3 টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   F3 = প্রস্থান",
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

static MUI_ENTRY bnINLicensePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        6,
        "লাইসেন্স:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        8,
        "ReactOS সিস্টেমটি GNU GPL-এর শর্তাবলীর অধীনে লাইসেন্সকৃত, যার",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        9,
        "কিছু অংশে X11, BSD এবং GNU LGPL-এর মতো অন্যান্য সংগতিপূর্ণ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "লাইসেন্সের কোড অন্তর্ভুক্ত রয়েছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "ReactOS সিস্টেমের অংশ এমন সব সফটওয়্যার তাই GNU GPL-এর",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "অধীনে প্রকাশিত হয়, পাশাপাশি মূল লাইসেন্সও বজায়",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "রাখা হয়।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "এই সফটওয়্যারে কোনো ওয়ারেন্টি বা ব্যবহারে নিষেধাজ্ঞা নেই,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        16,
        "প্রযোজ্য স্থানীয় ও আন্তর্জাতিক আইন ব্যতীত। ReactOS-এর লাইসেন্স",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "কেবল তৃতীয় পক্ষের কাছে বিতরণের ক্ষেত্রে প্রযোজ্য।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "যদি কোনো কারণে আপনি ReactOS-এর সাথে GNU General Public",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "License-এর একটি কপি না পেয়ে থাকেন, তাহলে দেখুন",
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
        "ওয়ারেন্টি:",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "এটি একটি মুক্ত সফটওয়্যার; কপি করার শর্তাবলীর জন্য সোর্স দেখুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "এর কোনো ওয়ারেন্টি নেই; এমনকি বাণিজ্যিক উপযোগিতা বা",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        26,
        "নির্দিষ্ট উদ্দেশ্যে উপযুক্ততার জন্যও নয়",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = ফিরে যান",
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

static MUI_ENTRY bnINDevicePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "নিচের তালিকায় বর্তমান ডিভাইস সেটিংস দেখানো হয়েছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        24,
        11,
        "কম্পিউটার:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        12,
        "ডিসপ্লে:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        13,
        "কীবোর্ড:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        14,
        "কীবোর্ড লেআউট:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        24,
        16,
        "গ্রহণ করুন:",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_RIGHT,
        TEXT_ID_STATIC
    },
    {
        25,
        16, "এই ডিভাইস সেটিংসগুলো গ্রহণ করুন",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "UP বা DOWN কী টিপে একটি এন্ট্রি নির্বাচন করে আপনি",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        20,
        "হার্ডওয়্যার সেটিংস পরিবর্তন করতে পারেন। তারপর বিকল্প সেটিংস",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "নির্বাচন করতে ENTER কী টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        23,
        "যখন সব সেটিংস সঠিক হবে, তখন \"এই ডিভাইস সেটিংসগুলো গ্রহণ করুন\"",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        24,
        "নির্বাচন করে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   F3 = প্রস্থান",
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

static MUI_ENTRY bnINRepairPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS সেটআপ এখনও প্রাথমিক ডেভেলপমেন্ট পর্যায়ে রয়েছে। এটি এখনও",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "একটি সম্পূর্ণ কার্যকর সেটআপ অ্যাপ্লিকেশনের সব ফাংশন সমর্থন করে না।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "মেরামত ফাংশনগুলো এখনও বাস্তবায়িত হয়নি।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  OS আপডেট করতে U টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  Recovery Console-এর জন্য R টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  মূল পাতায় ফিরে যেতে ESC টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  আপনার কম্পিউটার রিবুট করতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ESC = মূল পাতা  U = আপডেট  R = পুনরুদ্ধার  ENTER = রিবুট",
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

static MUI_ENTRY bnINUpgradePageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ReactOS সেটআপ নিচে তালিকাভুক্ত উপলব্ধ ReactOS ইনস্টলেশনগুলোর",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "মধ্যে একটি আপগ্রেড করতে পারে, অথবা কোনো ReactOS ইনস্টলেশন ক্ষতিগ্রস্ত হলে সেটআপ প্রোগ্রাম",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "এটি মেরামত করার চেষ্টা করতে পারে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "সব মেরামত ফাংশন এখনও বাস্তবায়িত হয়নি।",
        TEXT_STYLE_HIGHLIGHT,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  একটি OS ইনস্টলেশন নির্বাচন করতে UP বা DOWN টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  নির্বাচিত OS ইনস্টলেশন আপগ্রেড করতে U টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  নতুন ইনস্টলেশনের সাথে এগিয়ে যেতে ESC টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  ReactOS ইনস্টল না করে প্রস্থান করতে F3 টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "U = আপগ্রেড   ESC = আপগ্রেড করবেন না   F3 = প্রস্থান",
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

static MUI_ENTRY bnINComputerPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "আপনি ইনস্টল করার কম্পিউটারের ধরন পরিবর্তন করতে চান।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  পছন্দের কম্পিউটারের ধরন নির্বাচন করতে UP বা DOWN কী টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   তারপর ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  পরিবর্তন না করে আগের পাতায় ফিরে যেতে ESC কী টিপুন,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   কম্পিউটারের ধরন অপরিবর্তিত থাকবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   ESC = বাতিল   F3 = প্রস্থান",
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

static MUI_ENTRY bnINFlushPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "সিস্টেম এখন নিশ্চিত করছে যে সব ডেটা আপনার ডিস্কে সংরক্ষিত হয়েছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "এতে কিছুটা সময় লাগতে পারে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "সম্পন্ন হলে, আপনার কম্পিউটার স্বয়ংক্রিয়ভাবে রিবুট হবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ক্যাশ ফ্লাশ করা হচ্ছে",
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

static MUI_ENTRY bnINQuitPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS সম্পূর্ণভাবে ইনস্টল করা হয়নি।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "ড্রাইভ A: থেকে ফ্লপি ডিস্ক বের করুন এবং",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "CD-ড্রাইভ থেকে সব CD-ROM বের করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "আপনার কম্পিউটার রিবুট করতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "অনুগ্রহ করে অপেক্ষা করুন...",
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

static MUI_ENTRY bnINDisplayPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "আপনি ইনস্টল করার ডিসপ্লের ধরন পরিবর্তন করতে চান।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
         "\x07  পছন্দের ডিসপ্লের ধরন নির্বাচন করতে UP বা DOWN কী টিপুন।",
         TEXT_STYLE_NORMAL,
         TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   তারপর ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  পরিবর্তন না করে আগের পাতায় ফিরে যেতে ESC কী টিপুন,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   ডিসপ্লের ধরন অপরিবর্তিত থাকবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   ESC = বাতিল   F3 = প্রস্থান",
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

static MUI_ENTRY bnINSuccessPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        10,
        6,
        "ReactOS-এর মৌলিক উপাদানগুলো সফলভাবে ইনস্টল করা হয়েছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        8,
        "ড্রাইভ A: থেকে ফ্লপি ডিস্ক বের করুন এবং",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        9,
        "CD-ড্রাইভ থেকে সব CD-ROM বের করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        10,
        11,
        "আপনার কম্পিউটার রিবুট করতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = কম্পিউটার রিবুট করুন",
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

static MUI_ENTRY bnINSelectPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "নিচের তালিকায় বিদ্যমান পার্টিশন এবং নতুন পার্টিশনের জন্য",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "অব্যবহৃত ডিস্ক স্থান দেখানো হয়েছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "\x07  তালিকার একটি এন্ট্রি নির্বাচন করতে UP বা DOWN টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  নির্বাচিত পার্টিশনে ReactOS ইনস্টল করতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "\x07  একটি প্রাইমারি/লজিক্যাল পার্টিশন তৈরি করতে C টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        17,
        "\x07  একটি এক্সটেন্ডেড পার্টিশন তৈরি করতে E টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        19,
        "\x07  একটি বিদ্যমান পার্টিশন মুছতে D টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "অনুগ্রহ করে অপেক্ষা করুন...",
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

static MUI_ENTRY bnINChangeSystemPartition[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "আপনার কম্পিউটারের বর্তমান সিস্টেম পার্টিশনটি",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "(সিস্টেম ডিস্কে)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "এমন একটি ফরম্যাট ব্যবহার করছে যা ReactOS সমর্থন করে না।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        18,
        "ReactOS সফলভাবে ইনস্টল করার জন্য, সেটআপ প্রোগ্রামকে বর্তমান",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        19,
        "সিস্টেম পার্টিশনটি একটি নতুন পার্টিশনে পরিবর্তন করতে হবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        21,
        "নতুন প্রার্থী সিস্টেম পার্টিশনটি হলো:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "\x07  এই নির্বাচন গ্রহণ করতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        27,
        "\x07  সিস্টেম পার্টিশন হাতে পরিবর্তন করতে, পার্টিশন নির্বাচনের",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        28,
        "   তালিকায় ফিরে যেতে ESC টিপুন, তারপর সিস্টেম ডিস্কে একটি নতুন",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        29,
        "   সিস্টেম পার্টিশন নির্বাচন বা তৈরি করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        32,
        "যদি অন্য কোনো অপারেটিং সিস্টেম মূল সিস্টেম পার্টিশনের উপর",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        33,
        "নির্ভরশীল থাকে, তাহলে আপনাকে সেগুলো নতুন সিস্টেম পার্টিশনের",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        34,
        "জন্য পুনঃকনফিগার করতে হতে পারে, অথবা ReactOS ইনস্টলেশন শেষ",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        35,
        "হওয়ার পর সিস্টেম পার্টিশনটি মূল অবস্থায় ফিরিয়ে আনতে হতে পারে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   ESC = বাতিল",
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

static MUI_ENTRY bnINConfirmDeleteSystemPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "আপনি সিস্টেম পার্টিশন মুছে ফেলার সিদ্ধান্ত নিয়েছেন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        10,
        "সিস্টেম পার্টিশনে ডায়াগনস্টিক প্রোগ্রাম, হার্ডওয়্যার কনফিগারেশন",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        11,
        "প্রোগ্রাম, অপারেটিং সিস্টেম চালু করার প্রোগ্রাম (যেমন ReactOS)",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        12,
        "অথবা হার্ডওয়্যার নির্মাতা প্রদত্ত অন্য প্রোগ্রাম থাকতে পারে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "একটি সিস্টেম পার্টিশন তখনই মুছবেন যখন আপনি নিশ্চিত হবেন যে",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "পার্টিশনে এমন কোনো প্রোগ্রাম নেই, অথবা আপনি নিশ্চিতভাবে সেগুলো মুছতে চান।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "পার্টিশনটি মুছে ফেললে, ReactOS সেটআপ শেষ না হওয়া পর্যন্ত আপনি",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "হার্ডডিস্ক থেকে কম্পিউটার বুট করতে পারবেন না।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        20,
        "\x07  সিস্টেম পার্টিশন মুছতে ENTER টিপুন। পার্টিশনটি মুছে",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "   ফেলার বিষয়টি পরে আবার নিশ্চিত করতে আপনাকে বলা হবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        24,
        "\x07  আগের পাতায় ফিরে যেতে ESC টিপুন। পার্টিশনটি",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        25,
        "   মুছে ফেলা হবে না।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER=এগিয়ে যান  ESC=বাতিল",
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

static MUI_ENTRY bnINFormatPartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "পার্টিশন ফরম্যাট করুন",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "সেটআপ এখন পার্টিশনটি ফরম্যাট করবে। এগিয়ে যেতে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_FORMAT_PROMPT
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   F3 = প্রস্থান",
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

static MUI_ENTRY bnINCheckFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "সেটআপ এখন নির্বাচিত পার্টিশনটি পরীক্ষা করছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "অনুগ্রহ করে অপেক্ষা করুন...",
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

static MUI_ENTRY bnINInstallDirectoryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "সেটআপ নির্বাচিত পার্টিশনে ReactOS ফাইল ইনস্টল করে। আপনি যেখানে",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        9,
        "ReactOS ইনস্টল করতে চান, এমন একটি ডিরেক্টরি নির্বাচন করুন:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "প্রস্তাবিত ডিরেক্টরি পরিবর্তন করতে, অক্ষর মুছতে BACKSPACE টিপুন",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        15,
        "এবং তারপর আপনি যেখানে ReactOS ইনস্টল করতে চান তার",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        16,
        "ডিরেক্টরির নাম টাইপ করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   F3 = প্রস্থান",
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

static MUI_ENTRY bnINFileCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        0,
        12,
        "অনুগ্রহ করে অপেক্ষা করুন, ReactOS সেটআপ আপনার ReactOS",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        13,
        "ইনস্টলেশন ফোল্ডারে ফাইল কপি করছে।",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        0,
        14,
        "এতে সম্পন্ন হতে কয়েক মিনিট সময় লাগতে পারে।",
        TEXT_STYLE_NORMAL | TEXT_ALIGN_CENTER,
        TEXT_ID_STATIC
    },
    {
        50,
        0,
        "\xB3 অনুগ্রহ করে অপেক্ষা করুন...    ",
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

static MUI_ENTRY bnINBootLoaderSelectPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "সেটআপ কোথায় বুটলোডার ইনস্টল করবে তা নির্বাচন করুন:",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        12,
        "হার্ডডিস্কে বুটলোডার ইনস্টল করুন (MBR এবং VBR)।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "হার্ডডিস্কে বুটলোডার ইনস্টল করুন (শুধু VBR)।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "একটি ফ্লপি ডিস্কে বুটলোডার ইনস্টল করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        15,
        "বুটলোডার ইনস্টলেশন এড়িয়ে যান।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   F3 = প্রস্থান",
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

static MUI_ENTRY bnINBootLoaderInstallPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "সেটআপ বুটলোডার ইনস্টল করছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "মিডিয়ায় বুটলোডার ইনস্টল করা হচ্ছে, অনুগ্রহ করে অপেক্ষা করুন...",
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

static MUI_ENTRY bnINBootLoaderRemovableDiskPageEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "সেটআপ বুটলোডার ইনস্টল করছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        13,
        "ড্রাইভ A:-এ একটি ফরম্যাটকৃত ফ্লপি ডিস্ক প্রবেশ করান",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        6,
        14,
        "নির্বাচন করে ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   F3 = প্রস্থান",
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

static MUI_ENTRY bnINKeyboardSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "আপনি ইনস্টল করার কীবোর্ডের ধরন পরিবর্তন করতে চান।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  পছন্দের কীবোর্ডের ধরন নির্বাচন করতে UP বা DOWN কী টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "   তারপর ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  পরিবর্তন না করে আগের পাতায় ফিরে যেতে ESC কী টিপুন,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   কীবোর্ডের ধরন অপরিবর্তিত থাকবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   ESC = বাতিল   F3 = প্রস্থান",
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

static MUI_ENTRY bnINLayoutSettingsEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "ডিফল্টভাবে ইনস্টল করার জন্য একটি লেআউট নির্বাচন করুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        10,
        "\x07  পছন্দের কীবোর্ড লেআউট নির্বাচন করতে UP বা DOWN কী",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        11,
        "    টিপুন। তারপর ENTER টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        13,
        "\x07  পরিবর্তন না করে আগের পাতায় ফিরে যেতে ESC কী টিপুন,",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        14,
        "   কীবোর্ড লেআউট অপরিবর্তিত থাকবে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   ESC = বাতিল   F3 = প্রস্থান",
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

static MUI_ENTRY bnINPrepareCopyEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "সেটআপ ReactOS ফাইল কপি করার জন্য আপনার কম্পিউটার প্রস্তুত করছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "ফাইল কপির তালিকা তৈরি করা হচ্ছে...",
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

static MUI_ENTRY bnINSelectFSEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        17,
        "নিচের তালিকা থেকে একটি ফাইল সিস্টেম নির্বাচন করুন।",
        0
    },
    {
        8,
        19,
        "\x07  একটি ফাইল সিস্টেম নির্বাচন করতে UP বা DOWN টিপুন।",
        0
    },
    {
        8,
        21,
        "\x07  পার্টিশনটি ফরম্যাট করতে ENTER টিপুন।",
        0
    },
    {
        8,
        23,
        "\x07  অন্য একটি পার্টিশন নির্বাচন করতে ESC টিপুন।",
        0
    },
    {
        0,
        0,
        "ENTER = এগিয়ে যান   ESC = বাতিল   F3 = প্রস্থান",
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

static MUI_ENTRY bnINDeletePartitionEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "আপনি যে পার্টিশনটি মুছে ফেলার সিদ্ধান্ত নিয়েছেন",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        18,
        "\x07  পার্টিশনটি মুছতে L টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        11,
        19,
        "সতর্কতা: এই পার্টিশনের সব ডেটা মুছে যাবে!",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        8,
        21,
        "\x07  বাতিল করতে ESC টিপুন।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "L = পার্টিশন মুছুন   ESC = বাতিল   F3 = প্রস্থান",
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

static MUI_ENTRY bnINRegistryEntries[] =
{
    {
        4,
        3,
        " ReactOS " KERNEL_VERSION_STR " সেটআপ ",
        TEXT_STYLE_UNDERLINE,
        TEXT_ID_STATIC
    },
    {
        6,
        8,
        "সেটআপ সিস্টেম কনফিগারেশন আপডেট করছে।",
        TEXT_STYLE_NORMAL,
        TEXT_ID_STATIC
    },
    {
        0,
        0,
        "রেজিস্ট্রি হাইভ তৈরি করা হচ্ছে...",
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

MUI_ERROR bnINErrorEntries[] =
{
    {
        // NOT_AN_ERROR
        "সফল\n"
    },
    {
        // ERROR_NOT_INSTALLED
        "ReactOS আপনার কম্পিউটারে সম্পূর্ণভাবে ইনস্টল করা হয়নি।\n"
        "আপনি এখন সেটআপ বন্ধ করলে, ReactOS ইনস্টল করতে আবার\n"
        "সেটআপ চালাতে হবে।\n"
        "\n"
        "  \x07  সেটআপ চালিয়ে যেতে ENTER টিপুন।\n"
        "  \x07  সেটআপ বন্ধ করতে F3 টিপুন।",
        "F3 = প্রস্থান  ENTER = এগিয়ে যান"
    },
    {
        // ERROR_NO_BUILD_PATH
        "ReactOS ইনস্টলেশন ডিরেক্টরির জন্য ইনস্টলেশন পাথ তৈরি করতে ব্যর্থ হয়েছে!\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_SOURCE_PATH
        "ইনস্টলেশন সোর্স ধারণ করা পার্টিশনটি আপনি মুছতে পারবেন না!\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_SOURCE_DIR
        "আপনি ইনস্টলেশন সোর্স ডিরেক্টরির মধ্যে ReactOS ইনস্টল করতে পারবেন না!\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_NO_HDD
        "সেটআপ কোনো হার্ডডিস্ক খুঁজে পায়নি।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_NO_SOURCE_DRIVE
        "সেটআপ তার সোর্স ড্রাইভ খুঁজে পায়নি।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_LOAD_TXTSETUPSIF
        "সেটআপ TXTSETUP.SIF ফাইলটি লোড করতে ব্যর্থ হয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_CORRUPT_TXTSETUPSIF
        "সেটআপ একটি ক্ষতিগ্রস্ত TXTSETUP.SIF খুঁজে পেয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_SIGNATURE_TXTSETUPSIF,
        "সেটআপ TXTSETUP.SIF-এ একটি অবৈধ সিগনেচার খুঁজে পেয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_DRIVE_INFORMATION
        "সেটআপ সিস্টেম ড্রাইভের তথ্য সংগ্রহ করতে পারেনি।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_WRITE_BOOT,
        "সেটআপ সিস্টেম পার্টিশনে %S বুটকোড ইনস্টল করতে ব্যর্থ হয়েছে।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_LOAD_COMPUTER,
        "সেটআপ কম্পিউটারের ধরনের তালিকা লোড করতে ব্যর্থ হয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_LOAD_DISPLAY,
        "সেটআপ ডিসপ্লে সেটিংসের তালিকা লোড করতে ব্যর্থ হয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_LOAD_KEYBOARD,
        "সেটআপ কীবোর্ডের ধরনের তালিকা লোড করতে ব্যর্থ হয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_LOAD_KBLAYOUT,
        "সেটআপ কীবোর্ড লেআউটের তালিকা লোড করতে ব্যর্থ হয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_WARN_PARTITION,
        "সেটআপ দেখেছে অন্তত একটি হার্ডডিস্কে এমন একটি সংগতিহীন\n"
        "পার্টিশন টেবিল রয়েছে যা সঠিকভাবে পরিচালনা করা সম্ভব নয়!\n"
        "\n"
        "পার্টিশন তৈরি বা মুছে ফেললে পার্টিশন টেবিল নষ্ট হতে পারে।\n"
        "\n"
        "  \x07  সেটআপ বন্ধ করতে F3 টিপুন।\n"
        "  \x07  এগিয়ে যেতে ENTER টিপুন।",
        "F3 = প্রস্থান  ENTER = এগিয়ে যান"
    },
    {
        // ERROR_NEW_PARTITION,
        "একটি বিদ্যমান পার্টিশনের ভেতরে আপনি\n"
        "নতুন পার্টিশন তৈরি করতে পারবেন না!\n"
        "\n"
        "  * চালিয়ে যেতে কোনো কী টিপুন।",
        NULL
    },
    {
        // ERROR_INSTALL_BOOTCODE,
        "সেটআপ সিস্টেম পার্টিশনে %S বুটকোড ইনস্টল করতে ব্যর্থ হয়েছে।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_NO_FLOPPY,
        "ড্রাইভ A:-এ কোনো ডিস্ক নেই।",
        "ENTER = এগিয়ে যান"
    },
    {
        // ERROR_UPDATE_KBSETTINGS,
        "সেটআপ কীবোর্ড লেআউট সেটিংস আপডেট করতে ব্যর্থ হয়েছে।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_UPDATE_DISPLAY_SETTINGS,
        "সেটআপ ডিসপ্লে রেজিস্ট্রি সেটিংস আপডেট করতে ব্যর্থ হয়েছে।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_IMPORT_HIVE,
        "সেটআপ একটি হাইভ ফাইল ইম্পোর্ট করতে ব্যর্থ হয়েছে।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_FIND_REGISTRY
        "সেটআপ রেজিস্ট্রি ডেটা ফাইল খুঁজে পায়নি।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_CREATE_HIVE,
        "সেটআপ রেজিস্ট্রি হাইভ তৈরি করতে ব্যর্থ হয়েছে।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_INITIALIZE_REGISTRY,
        "সেটআপ রেজিস্ট্রি শুরু করতে ব্যর্থ হয়েছে।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_INVALID_CABINET_INF,
        "ক্যাবিনেটে কোনো বৈধ inf ফাইল নেই।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_CABINET_MISSING,
        "ক্যাবিনেট খুঁজে পাওয়া যায়নি।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_CABINET_SCRIPT,
        "ক্যাবিনেটে কোনো সেটআপ স্ক্রিপ্ট নেই।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_COPY_QUEUE,
        "সেটআপ কপি ফাইল কিউ খুলতে ব্যর্থ হয়েছে।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_CREATE_DIR,
        "সেটআপ ইনস্টলেশন ডিরেক্টরিগুলো তৈরি করতে পারেনি।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_TXTSETUP_SECTION,
        "সেটআপ '%S' সেকশন\n"
        "TXTSETUP.SIF-এ খুঁজে পায়নি।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_CABINET_SECTION,
        "সেটআপ '%S' সেকশন\n"
        "ক্যাবিনেটে খুঁজে পায়নি।\n",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_CREATE_INSTALL_DIR
        "সেটআপ ইনস্টলেশন ডিরেক্টরি তৈরি করতে পারেনি।",
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_WRITE_PTABLE,
        "সেটআপ পার্টিশন টেবিল লিখতে ব্যর্থ হয়েছে।\n"
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_ADDING_CODEPAGE,
        "সেটআপ রেজিস্ট্রিতে কোডপেজ যুক্ত করতে ব্যর্থ হয়েছে।\n"
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_UPDATE_LOCALESETTINGS,
        "সেটআপ সিস্টেম লোকেল সেট করতে পারেনি।\n"
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_ADDING_KBLAYOUTS,
        "সেটআপ রেজিস্ট্রিতে কীবোর্ড লেআউট যুক্ত করতে ব্যর্থ হয়েছে।\n"
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_UPDATE_GEOID,
        "সেটআপ জিও আইডি সেট করতে পারেনি।\n"
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        // ERROR_DIRECTORY_NAME,
        "ডিরেক্টরির নাম অবৈধ।\n"
        "\n"
        "  * চালিয়ে যেতে কোনো কী টিপুন।"
    },
    {
        // ERROR_INSUFFICIENT_PARTITION_SIZE,
        "নির্বাচিত পার্টিশনটি ReactOS ইনস্টল করার জন্য যথেষ্ট বড় নয়।\n"
        "ইনস্টল পার্টিশনের আকার কমপক্ষে %lu MB হতে হবে।\n"
        "\n"
        "  * চালিয়ে যেতে কোনো কী টিপুন।",
        NULL
    },
    {
        // ERROR_PARTITION_TABLE_FULL,
        "আপনি এই ডিস্কের পার্টিশন টেবিলে নতুন প্রাইমারি বা এক্সটেন্ডেড\n"
        "পার্টিশন তৈরি করতে পারবেন না, কারণ পার্টিশন টেবিলটি পূর্ণ।\n"
        "\n"
        "  * চালিয়ে যেতে কোনো কী টিপুন।"
    },
    {
        // ERROR_ONLY_ONE_EXTENDED,
        "প্রতি ডিস্কে আপনি একাধিক এক্সটেন্ডেড পার্টিশন তৈরি করতে পারবেন না।\n"
        "\n"
        "  * চালিয়ে যেতে কোনো কী টিপুন।"
    },
    {
        // ERROR_FORMATTING_PARTITION,
        "সেটআপ পার্টিশনটি ফরম্যাট করতে পারছে না:\n"
        " %S\n"
        "\n"
        "ENTER = কম্পিউটার রিবুট করুন"
    },
    {
        NULL,
        NULL
    }
};

MUI_PAGE bnINPages[] =
{
    {
        SETUP_INIT_PAGE,
        bnINSetupInitPageEntries
    },
    {
        LANGUAGE_PAGE,
        bnINLanguagePageEntries
    },
    {
        WELCOME_PAGE,
        bnINWelcomePageEntries
    },
    {
        INSTALL_INTRO_PAGE,
        bnINIntroPageEntries
    },
    {
        LICENSE_PAGE,
        bnINLicensePageEntries
    },
    {
        DEVICE_SETTINGS_PAGE,
        bnINDevicePageEntries
    },
    {
        REPAIR_INTRO_PAGE,
        bnINRepairPageEntries
    },
    {
        UPGRADE_REPAIR_PAGE,
        bnINUpgradePageEntries
    },
    {
        COMPUTER_SETTINGS_PAGE,
        bnINComputerPageEntries
    },
    {
        DISPLAY_SETTINGS_PAGE,
        bnINDisplayPageEntries
    },
    {
        FLUSH_PAGE,
        bnINFlushPageEntries
    },
    {
        SELECT_PARTITION_PAGE,
        bnINSelectPartitionEntries
    },
    {
        CHANGE_SYSTEM_PARTITION,
        bnINChangeSystemPartition
    },
    {
        CONFIRM_DELETE_SYSTEM_PARTITION_PAGE,
        bnINConfirmDeleteSystemPartitionEntries
    },
    {
        SELECT_FILE_SYSTEM_PAGE,
        bnINSelectFSEntries
    },
    {
        FORMAT_PARTITION_PAGE,
        bnINFormatPartitionEntries
    },
    {
        CHECK_FILE_SYSTEM_PAGE,
        bnINCheckFSEntries
    },
    {
        DELETE_PARTITION_PAGE,
        bnINDeletePartitionEntries
    },
    {
        INSTALL_DIRECTORY_PAGE,
        bnINInstallDirectoryEntries
    },
    {
        PREPARE_COPY_PAGE,
        bnINPrepareCopyEntries
    },
    {
        FILE_COPY_PAGE,
        bnINFileCopyEntries
    },
    {
        KEYBOARD_SETTINGS_PAGE,
        bnINKeyboardSettingsEntries
    },
    {
        BOOTLOADER_SELECT_PAGE,
        bnINBootLoaderSelectPageEntries
    },
    {
        LAYOUT_SETTINGS_PAGE,
        bnINLayoutSettingsEntries
    },
    {
        QUIT_PAGE,
        bnINQuitPageEntries
    },
    {
        SUCCESS_PAGE,
        bnINSuccessPageEntries
    },
    {
        BOOTLOADER_INSTALL_PAGE,
        bnINBootLoaderInstallPageEntries
    },
    {
        BOOTLOADER_REMOVABLE_DISK_PAGE,
        bnINBootLoaderRemovableDiskPageEntries
    },
    {
        REGISTRY_PAGE,
        bnINRegistryEntries
    },
    {
        -1,
        NULL
    }
};

MUI_STRING bnINStrings[] =
{
    {STRING_PLEASEWAIT,
     "   অনুগ্রহ করে অপেক্ষা করুন..."},
    {STRING_INSTALLCREATEPARTITION,
     "   ENTER = ইনস্টল   C = প্রাইমারি তৈরি করুন   E = এক্সটেন্ডেড তৈরি করুন   F3 = প্রস্থান"},
    {STRING_INSTALLCREATELOGICAL,
     "   ENTER = ইনস্টল   C = লজিক্যাল পার্টিশন তৈরি করুন   F3 = প্রস্থান"},
    {STRING_INSTALLDELETEPARTITION,
     "   ENTER = ইনস্টল   D = পার্টিশন মুছুন   F3 = প্রস্থান"},
    {STRING_DELETEPARTITION,
     "   D = পার্টিশন মুছুন   F3 = প্রস্থান"},
    {STRING_PARTITIONSIZE,
     "নতুন পার্টিশনের আকার:"},
    {STRING_CHOOSE_NEW_PARTITION,
     "আপনি যেখানে প্রাইমারি পার্টিশন তৈরি করার সিদ্ধান্ত নিয়েছেন"},
    {STRING_CHOOSE_NEW_EXTENDED_PARTITION,
     "আপনি যেখানে এক্সটেন্ডেড পার্টিশন তৈরি করার সিদ্ধান্ত নিয়েছেন"},
    {STRING_CHOOSE_NEW_LOGICAL_PARTITION,
     "আপনি যেখানে লজিক্যাল পার্টিশন তৈরি করার সিদ্ধান্ত নিয়েছেন"},
    {STRING_HDPARTSIZE,
    "নতুন পার্টিশনের আকার মেগাবাইটে লিখুন।"},
    {STRING_CREATEPARTITION,
     "   ENTER = পার্টিশন তৈরি করুন   ESC = বাতিল   F3 = প্রস্থান"},
    {STRING_NEWPARTITION,
    "সেটআপ একটি নতুন পার্টিশন তৈরি করেছে"},
    {STRING_PARTFORMAT,
    "এই পার্টিশনটি পরে ফরম্যাট করা হবে।"},
    {STRING_NONFORMATTEDPART,
    "আপনি একটি নতুন বা ফরম্যাট না করা পার্টিশনে ReactOS ইনস্টল করতে বেছে নিয়েছেন।"},
    {STRING_NONFORMATTEDSYSTEMPART,
    "সিস্টেম পার্টিশনটি এখনও ফরম্যাট করা হয়নি।"},
    {STRING_NONFORMATTEDOTHERPART,
    "নতুন পার্টিশনটি এখনও ফরম্যাট করা হয়নি।"},
    {STRING_INSTALLONPART,
    "সেটআপ যে পার্টিশনে ReactOS ইনস্টল করবে"},
    {STRING_CONTINUE,
    "ENTER = এগিয়ে যান"},
    {STRING_QUITCONTINUE,
    "F3 = প্রস্থান  ENTER = এগিয়ে যান"},
    {STRING_REBOOTCOMPUTER,
    "ENTER = কম্পিউটার রিবুট করুন"},
    {STRING_DELETING,
     "   ফাইল মুছে ফেলা হচ্ছে: %S"},
    {STRING_MOVING,
     "   ফাইল সরানো হচ্ছে: %S \xe2\x86\x92 %S"},
    {STRING_RENAMING,
     "   নাম পরিবর্তন করা হচ্ছে: %S \xe2\x86\x92 %S"},
    {STRING_COPYING,
     "   ফাইল কপি করা হচ্ছে: %S"},
    {STRING_SETUPCOPYINGFILES,
     "সেটআপ ফাইল কপি করছে..."},
    {STRING_REGHIVEUPDATE,
    "   রেজিস্ট্রি হাইভ আপডেট করা হচ্ছে..."},
    {STRING_IMPORTFILE,
    "   %S ইম্পোর্ট করা হচ্ছে..."},
    {STRING_DISPLAYSETTINGSUPDATE,
    "   ডিসপ্লে রেজিস্ট্রি সেটিংস আপডেট করা হচ্ছে..."},
    {STRING_LOCALESETTINGSUPDATE,
    "   লোকেল সেটিংস আপডেট করা হচ্ছে..."},
    {STRING_KEYBOARDSETTINGSUPDATE,
    "   কীবোর্ড লেআউট সেটিংস আপডেট করা হচ্ছে..."},
    {STRING_CODEPAGEINFOUPDATE,
    "   কোডপেজের তথ্য যুক্ত করা হচ্ছে..."},
    {STRING_DONE,
    "   সম্পন্ন..."},
    {STRING_REBOOTCOMPUTER2,
    "   ENTER = কম্পিউটার রিবুট করুন"},
    {STRING_REBOOTPROGRESSBAR,
    " আপনার কম্পিউটার %li সেকেন্ডে রিবুট হবে... "},
    {STRING_CONSOLEFAIL1,
    "কনসোল খোলা যাচ্ছে না\r\n\r\n"},
    {STRING_CONSOLEFAIL2,
    "এর সবচেয়ে সাধারণ কারণ USB কীবোর্ড ব্যবহার করা\r\n"},
    {STRING_CONSOLEFAIL3,
    "USB কীবোর্ড এখনও সম্পূর্ণভাবে সমর্থিত নয়\r\n"},
    {STRING_FORMATTINGPART,
    "সেটআপ পার্টিশনটি ফরম্যাট করছে..."},
    {STRING_CHECKINGDISK,
    "সেটআপ ডিস্কটি পরীক্ষা করছে..."},
    {STRING_FORMATDISK1,
    " পার্টিশনটি %S ফাইল সিস্টেম হিসেবে ফরম্যাট করুন (কুইক ফরম্যাট) "},
    {STRING_FORMATDISK2,
    " পার্টিশনটি %S ফাইল সিস্টেম হিসেবে ফরম্যাট করুন "},
    {STRING_KEEPFORMAT,
    " বর্তমান ফাইল সিস্টেম রাখুন (কোনো পরিবর্তন নয়) "},
    {STRING_HDDISK1,
    "%s।"},
    {STRING_HDDISK2,
    "%s-এ।"},
    {STRING_PARTTYPE,
    "ধরন 0x%02x"},
    {STRING_HDDINFO1,
    // "হার্ডডিস্ক %lu (%I64u %s), পোর্ট=%hu, বাস=%hu, আইডি=%hu (%wZ) [%s]"
    "%I64u %s হার্ডডিস্ক %lu (পোর্ট=%hu, বাস=%hu, আইডি=%hu) %wZ-এ [%s]"},
    {STRING_HDDINFO2,
    // "হার্ডডিস্ক %lu (%I64u %s), পোর্ট=%hu, বাস=%hu, আইডি=%hu [%s]"
    "%I64u %s হার্ডডিস্ক %lu (পোর্ট=%hu, বাস=%hu, আইডি=%hu) [%s]"},
    {STRING_UNPSPACE,
    "পার্টিশনহীন স্থান"},
    {STRING_MAXSIZE,
    "MB (সর্বোচ্চ %lu MB)"},
    {STRING_EXTENDED_PARTITION,
    "এক্সটেন্ডেড পার্টিশন"},
    {STRING_UNFORMATTED,
    "নতুন (ফরম্যাট করা হয়নি)"},
    {STRING_FORMATUNUSED,
    "অব্যবহৃত"},
    {STRING_FORMATUNKNOWN,
    "অজানা"},
    {STRING_KB,
    "KB"},
    {STRING_MB,
    "MB"},
    {STRING_GB,
    "GB"},
    {STRING_ADDKBLAYOUTS,
    "কীবোর্ড লেআউট যুক্ত করা হচ্ছে"},
    {0, 0}
};
