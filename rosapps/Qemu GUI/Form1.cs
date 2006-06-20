using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Globalization;
using System.IO;
using System.Diagnostics;

namespace Qemu_GUI
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class Form1 : System.Windows.Forms.Form
	{
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.TabControl HardDisk2;
        private System.Windows.Forms.RadioButton radioButton1;
        private System.Windows.Forms.RadioButton radioButton2;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.CheckBox checkBox1;
        private System.Windows.Forms.CheckBox checkBox2;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.TabPage tabPage3;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.CheckBox checkBox4;
        private System.Windows.Forms.Button button5;
        private System.Windows.Forms.Button button6;
        private System.Windows.Forms.CheckBox checkBox5;
        private System.Windows.Forms.CheckBox checkBox6;
        private System.Windows.Forms.CheckBox checkBox3;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.Button button8;
        private System.Windows.Forms.CheckBox checkBox8;
        private System.Windows.Forms.RadioButton radioButton3;
        private System.Windows.Forms.RadioButton radioButton4;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.TabPage HardDisk;
        private System.Windows.Forms.TabPage tabPage4;
        private System.Windows.Forms.TabPage tabPage5;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.CheckBox checkBox7;
        private System.Windows.Forms.CheckBox checkBox9;
        private System.Windows.Forms.CheckBox checkBox11;
        private System.Windows.Forms.CheckBox checkBox12;
        private System.Windows.Forms.TabPage tabPage6;
        private System.Windows.Forms.ComboBox comboBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox textBox3;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.RadioButton radioButton5;
        private System.Windows.Forms.RadioButton radioButton6;
        private System.Windows.Forms.GroupBox groupBox6;
        private System.Windows.Forms.RadioButton radioButton7;
        private System.Windows.Forms.RadioButton radioButton8;
        private System.Windows.Forms.RadioButton radioButton9;
        private System.Windows.Forms.RadioButton radioButton10;
        private System.Windows.Forms.Button button7;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Button button10;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Button button11;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.GroupBox groupBox8;
        private System.Windows.Forms.Button button12;
        private System.Windows.Forms.GroupBox groupBox9;
        private System.Windows.Forms.Button button13;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.TextBox textBox4;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage7;
        private System.Windows.Forms.TabPage tabPage8;
        private System.Windows.Forms.TextBox textBox5;
        private System.Windows.Forms.GroupBox groupBox10;
        private System.Windows.Forms.GroupBox groupBox11;
        private System.Windows.Forms.GroupBox groupBox7;
        private System.Windows.Forms.RadioButton radioButton11;
        private System.Windows.Forms.RadioButton radioButton12;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Button button15;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Button button16;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Button button17;
        private System.Windows.Forms.GroupBox groupBox12;
        private System.Windows.Forms.CheckBox checkBox10;
        private System.Windows.Forms.Button button18;
        private System.Windows.Forms.GroupBox groupBox13;
        private System.Windows.Forms.Button button19;
        private System.Windows.Forms.CheckBox checkBox13;
        private System.Windows.Forms.GroupBox groupBox14;
        private System.Windows.Forms.CheckBox checkBox14;
        private System.Windows.Forms.TextBox textBox6;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.GroupBox groupBox15;
        private System.Windows.Forms.CheckBox checkBox15;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.TextBox textBox7;
        private System.Windows.Forms.GroupBox groupBox16;
        private System.Windows.Forms.Button button20;
        private System.Windows.Forms.CheckBox checkBox16;
        private System.Windows.Forms.ComboBox comboBox2;
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog1;
        private System.Windows.Forms.OpenFileDialog openFile;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.TabPage tabPage9;
        private System.Windows.Forms.ComboBox comboBox3;
        private System.Windows.Forms.Label label11;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form1()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(Form1));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.radioButton2 = new System.Windows.Forms.RadioButton();
            this.radioButton1 = new System.Windows.Forms.RadioButton();
            this.HardDisk2 = new System.Windows.Forms.TabControl();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.groupBox7 = new System.Windows.Forms.GroupBox();
            this.radioButton11 = new System.Windows.Forms.RadioButton();
            this.radioButton12 = new System.Windows.Forms.RadioButton();
            this.groupBox6 = new System.Windows.Forms.GroupBox();
            this.comboBox2 = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.radioButton5 = new System.Windows.Forms.RadioButton();
            this.label4 = new System.Windows.Forms.Label();
            this.radioButton6 = new System.Windows.Forms.RadioButton();
            this.textBox3 = new System.Windows.Forms.TextBox();
            this.comboBox1 = new System.Windows.Forms.ComboBox();
            this.groupBox10 = new System.Windows.Forms.GroupBox();
            this.radioButton7 = new System.Windows.Forms.RadioButton();
            this.radioButton8 = new System.Windows.Forms.RadioButton();
            this.groupBox11 = new System.Windows.Forms.GroupBox();
            this.radioButton9 = new System.Windows.Forms.RadioButton();
            this.radioButton10 = new System.Windows.Forms.RadioButton();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.radioButton3 = new System.Windows.Forms.RadioButton();
            this.button8 = new System.Windows.Forms.Button();
            this.checkBox8 = new System.Windows.Forms.CheckBox();
            this.radioButton4 = new System.Windows.Forms.RadioButton();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.button2 = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.checkBox2 = new System.Windows.Forms.CheckBox();
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.HardDisk = new System.Windows.Forms.TabPage();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.button5 = new System.Windows.Forms.Button();
            this.button6 = new System.Windows.Forms.Button();
            this.checkBox5 = new System.Windows.Forms.CheckBox();
            this.checkBox6 = new System.Windows.Forms.CheckBox();
            this.button3 = new System.Windows.Forms.Button();
            this.button4 = new System.Windows.Forms.Button();
            this.checkBox3 = new System.Windows.Forms.CheckBox();
            this.checkBox4 = new System.Windows.Forms.CheckBox();
            this.tabPage6 = new System.Windows.Forms.TabPage();
            this.groupBox9 = new System.Windows.Forms.GroupBox();
            this.textBox4 = new System.Windows.Forms.TextBox();
            this.label12 = new System.Windows.Forms.Label();
            this.button13 = new System.Windows.Forms.Button();
            this.groupBox8 = new System.Windows.Forms.GroupBox();
            this.button12 = new System.Windows.Forms.Button();
            this.label8 = new System.Windows.Forms.Label();
            this.button10 = new System.Windows.Forms.Button();
            this.label9 = new System.Windows.Forms.Label();
            this.button11 = new System.Windows.Forms.Button();
            this.label10 = new System.Windows.Forms.Label();
            this.tabPage5 = new System.Windows.Forms.TabPage();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.checkBox9 = new System.Windows.Forms.CheckBox();
            this.checkBox7 = new System.Windows.Forms.CheckBox();
            this.checkBox11 = new System.Windows.Forms.CheckBox();
            this.checkBox12 = new System.Windows.Forms.CheckBox();
            this.tabPage7 = new System.Windows.Forms.TabPage();
            this.groupBox15 = new System.Windows.Forms.GroupBox();
            this.textBox7 = new System.Windows.Forms.TextBox();
            this.label14 = new System.Windows.Forms.Label();
            this.checkBox15 = new System.Windows.Forms.CheckBox();
            this.groupBox14 = new System.Windows.Forms.GroupBox();
            this.label13 = new System.Windows.Forms.Label();
            this.textBox6 = new System.Windows.Forms.TextBox();
            this.checkBox14 = new System.Windows.Forms.CheckBox();
            this.groupBox12 = new System.Windows.Forms.GroupBox();
            this.button18 = new System.Windows.Forms.Button();
            this.checkBox10 = new System.Windows.Forms.CheckBox();
            this.groupBox13 = new System.Windows.Forms.GroupBox();
            this.button19 = new System.Windows.Forms.Button();
            this.checkBox13 = new System.Windows.Forms.CheckBox();
            this.groupBox16 = new System.Windows.Forms.GroupBox();
            this.button20 = new System.Windows.Forms.Button();
            this.checkBox16 = new System.Windows.Forms.CheckBox();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.tabPage8 = new System.Windows.Forms.TabPage();
            this.button17 = new System.Windows.Forms.Button();
            this.label7 = new System.Windows.Forms.Label();
            this.button16 = new System.Windows.Forms.Button();
            this.label6 = new System.Windows.Forms.Label();
            this.button15 = new System.Windows.Forms.Button();
            this.label5 = new System.Windows.Forms.Label();
            this.textBox5 = new System.Windows.Forms.TextBox();
            this.tabPage9 = new System.Windows.Forms.TabPage();
            this.button7 = new System.Windows.Forms.Button();
            this.openFile = new System.Windows.Forms.OpenFileDialog();
            this.folderBrowserDialog1 = new System.Windows.Forms.FolderBrowserDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.comboBox3 = new System.Windows.Forms.ComboBox();
            this.label11 = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            this.HardDisk2.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.groupBox7.SuspendLayout();
            this.groupBox6.SuspendLayout();
            this.groupBox10.SuspendLayout();
            this.groupBox11.SuspendLayout();
            this.tabPage4.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.HardDisk.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.tabPage6.SuspendLayout();
            this.groupBox9.SuspendLayout();
            this.groupBox8.SuspendLayout();
            this.tabPage5.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.tabPage7.SuspendLayout();
            this.groupBox15.SuspendLayout();
            this.groupBox14.SuspendLayout();
            this.groupBox12.SuspendLayout();
            this.groupBox13.SuspendLayout();
            this.groupBox16.SuspendLayout();
            this.tabPage8.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.radioButton2);
            this.groupBox1.Controls.Add(this.radioButton1);
            this.groupBox1.Location = new System.Drawing.Point(32, 8);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(280, 48);
            this.groupBox1.TabIndex = 2;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Machine";
            this.groupBox1.Enter += new System.EventHandler(this.groupBox1_Enter);
            // 
            // radioButton2
            // 
            this.radioButton2.CausesValidation = false;
            this.radioButton2.Location = new System.Drawing.Point(120, 16);
            this.radioButton2.Name = "radioButton2";
            this.radioButton2.TabIndex = 5;
            this.radioButton2.Text = "ISA Only PC";
            // 
            // radioButton1
            // 
            this.radioButton1.CausesValidation = false;
            this.radioButton1.Checked = true;
            this.radioButton1.Location = new System.Drawing.Point(8, 16);
            this.radioButton1.Name = "radioButton1";
            this.radioButton1.TabIndex = 4;
            this.radioButton1.TabStop = true;
            this.radioButton1.Text = "Standard PC";
            // 
            // HardDisk2
            // 
            this.HardDisk2.Controls.Add(this.tabPage2);
            this.HardDisk2.Controls.Add(this.tabPage4);
            this.HardDisk2.Controls.Add(this.tabPage3);
            this.HardDisk2.Controls.Add(this.HardDisk);
            this.HardDisk2.Controls.Add(this.tabPage6);
            this.HardDisk2.Controls.Add(this.tabPage5);
            this.HardDisk2.Controls.Add(this.tabPage7);
            this.HardDisk2.Controls.Add(this.tabPage1);
            this.HardDisk2.Controls.Add(this.tabPage8);
            this.HardDisk2.Controls.Add(this.tabPage9);
            this.HardDisk2.Location = new System.Drawing.Point(24, 8);
            this.HardDisk2.Name = "HardDisk2";
            this.HardDisk2.SelectedIndex = 0;
            this.HardDisk2.Size = new System.Drawing.Size(488, 248);
            this.HardDisk2.TabIndex = 3;
            this.HardDisk2.SelectedIndexChanged += new System.EventHandler(this.HardDisk2_SelectedIndexChanged);
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.groupBox7);
            this.tabPage2.Controls.Add(this.groupBox6);
            this.tabPage2.Controls.Add(this.groupBox1);
            this.tabPage2.Controls.Add(this.groupBox10);
            this.tabPage2.Controls.Add(this.groupBox11);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Size = new System.Drawing.Size(480, 222);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Misc";
            // 
            // groupBox7
            // 
            this.groupBox7.Controls.Add(this.radioButton11);
            this.groupBox7.Controls.Add(this.radioButton12);
            this.groupBox7.Location = new System.Drawing.Point(328, 160);
            this.groupBox7.Name = "groupBox7";
            this.groupBox7.Size = new System.Drawing.Size(120, 56);
            this.groupBox7.TabIndex = 20;
            this.groupBox7.TabStop = false;
            this.groupBox7.Text = "Kqemu";
            // 
            // radioButton11
            // 
            this.radioButton11.CausesValidation = false;
            this.radioButton11.Location = new System.Drawing.Point(8, 24);
            this.radioButton11.Name = "radioButton11";
            this.radioButton11.Size = new System.Drawing.Size(48, 24);
            this.radioButton11.TabIndex = 13;
            this.radioButton11.Text = "Yes";
            // 
            // radioButton12
            // 
            this.radioButton12.CausesValidation = false;
            this.radioButton12.Checked = true;
            this.radioButton12.Location = new System.Drawing.Point(56, 24);
            this.radioButton12.Name = "radioButton12";
            this.radioButton12.Size = new System.Drawing.Size(56, 24);
            this.radioButton12.TabIndex = 14;
            this.radioButton12.TabStop = true;
            this.radioButton12.Text = "No";
            // 
            // groupBox6
            // 
            this.groupBox6.Controls.Add(this.comboBox2);
            this.groupBox6.Controls.Add(this.label3);
            this.groupBox6.Controls.Add(this.label1);
            this.groupBox6.Controls.Add(this.label2);
            this.groupBox6.Controls.Add(this.radioButton5);
            this.groupBox6.Controls.Add(this.label4);
            this.groupBox6.Controls.Add(this.radioButton6);
            this.groupBox6.Controls.Add(this.textBox3);
            this.groupBox6.Controls.Add(this.comboBox1);
            this.groupBox6.Location = new System.Drawing.Point(32, 64);
            this.groupBox6.Name = "groupBox6";
            this.groupBox6.Size = new System.Drawing.Size(280, 152);
            this.groupBox6.TabIndex = 4;
            this.groupBox6.TabStop = false;
            // 
            // comboBox2
            // 
            this.comboBox2.Items.AddRange(new object[] {
                                                           "1",
                                                           "2",
                                                           "3",
                                                           "4",
                                                           "5",
                                                           "6",
                                                           "7",
                                                           "8",
                                                           "9",
                                                           "10",
                                                           "11",
                                                           "12",
                                                           "13",
                                                           "14",
                                                           "15",
                                                           "16",
                                                           "17",
                                                           "18",
                                                           "19",
                                                           "20",
                                                           "21",
                                                           "22",
                                                           "23",
                                                           "24",
                                                           "25",
                                                           "26",
                                                           "27",
                                                           "28",
                                                           "29",
                                                           "30",
                                                           "31",
                                                           "32",
                                                           "33",
                                                           "34",
                                                           "35",
                                                           "36",
                                                           "37",
                                                           "38",
                                                           "39",
                                                           "40",
                                                           "41",
                                                           "42",
                                                           "43",
                                                           "44",
                                                           "45",
                                                           "46",
                                                           "47",
                                                           "48",
                                                           "49",
                                                           "50",
                                                           "51",
                                                           "52",
                                                           "53",
                                                           "54",
                                                           "55",
                                                           "56",
                                                           "57",
                                                           "58",
                                                           "59",
                                                           "60",
                                                           "61",
                                                           "62",
                                                           "63",
                                                           "64",
                                                           "65",
                                                           "66",
                                                           "67",
                                                           "68",
                                                           "69",
                                                           "70",
                                                           "71",
                                                           "72",
                                                           "73",
                                                           "74",
                                                           "75",
                                                           "76",
                                                           "77",
                                                           "78",
                                                           "79",
                                                           "80",
                                                           "81",
                                                           "82",
                                                           "83",
                                                           "84",
                                                           "85",
                                                           "86",
                                                           "87",
                                                           "88",
                                                           "89",
                                                           "90",
                                                           "91",
                                                           "92",
                                                           "93",
                                                           "94",
                                                           "95",
                                                           "96",
                                                           "97",
                                                           "98",
                                                           "99",
                                                           "100",
                                                           "101",
                                                           "102",
                                                           "103",
                                                           "104",
                                                           "105",
                                                           "106",
                                                           "107",
                                                           "108",
                                                           "109",
                                                           "110",
                                                           "111",
                                                           "112",
                                                           "113",
                                                           "114",
                                                           "115",
                                                           "116",
                                                           "117",
                                                           "118",
                                                           "119",
                                                           "120",
                                                           "121",
                                                           "122",
                                                           "123",
                                                           "124",
                                                           "125",
                                                           "126",
                                                           "127",
                                                           "128",
                                                           "129",
                                                           "130",
                                                           "131",
                                                           "132",
                                                           "133",
                                                           "134",
                                                           "135",
                                                           "136",
                                                           "137",
                                                           "138",
                                                           "139",
                                                           "140",
                                                           "141",
                                                           "142",
                                                           "143",
                                                           "144",
                                                           "145",
                                                           "146",
                                                           "147",
                                                           "148",
                                                           "149",
                                                           "150",
                                                           "151",
                                                           "152",
                                                           "153",
                                                           "154",
                                                           "155",
                                                           "156",
                                                           "157",
                                                           "158",
                                                           "159",
                                                           "160",
                                                           "161",
                                                           "162",
                                                           "163",
                                                           "164",
                                                           "165",
                                                           "166",
                                                           "167",
                                                           "168",
                                                           "169",
                                                           "170",
                                                           "171",
                                                           "172",
                                                           "173",
                                                           "174",
                                                           "175",
                                                           "176",
                                                           "177",
                                                           "178",
                                                           "179",
                                                           "180",
                                                           "181",
                                                           "182",
                                                           "183",
                                                           "184",
                                                           "185",
                                                           "186",
                                                           "187",
                                                           "188",
                                                           "189",
                                                           "190",
                                                           "191",
                                                           "192",
                                                           "193",
                                                           "194",
                                                           "195",
                                                           "196",
                                                           "197",
                                                           "198",
                                                           "199",
                                                           "200",
                                                           "201",
                                                           "202",
                                                           "203",
                                                           "204",
                                                           "205",
                                                           "206",
                                                           "207",
                                                           "208",
                                                           "209",
                                                           "210",
                                                           "211",
                                                           "212",
                                                           "213",
                                                           "214",
                                                           "215",
                                                           "216",
                                                           "217",
                                                           "218",
                                                           "219",
                                                           "220",
                                                           "221",
                                                           "222",
                                                           "223",
                                                           "224",
                                                           "225",
                                                           "226",
                                                           "227",
                                                           "228",
                                                           "229",
                                                           "230",
                                                           "231",
                                                           "232",
                                                           "233",
                                                           "234",
                                                           "235",
                                                           "236",
                                                           "237",
                                                           "238",
                                                           "239",
                                                           "240",
                                                           "241",
                                                           "242",
                                                           "243",
                                                           "244",
                                                           "245",
                                                           "246",
                                                           "247",
                                                           "248",
                                                           "249",
                                                           "250",
                                                           "251",
                                                           "252",
                                                           "253",
                                                           "254",
                                                           "255"});
            this.comboBox2.Location = new System.Drawing.Point(16, 120);
            this.comboBox2.Name = "comboBox2";
            this.comboBox2.Size = new System.Drawing.Size(120, 21);
            this.comboBox2.TabIndex = 10;
            this.comboBox2.Text = "1";
            // 
            // label3
            // 
            this.label3.Location = new System.Drawing.Point(152, 24);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(112, 23);
            this.label3.TabIndex = 6;
            this.label3.Text = "Memmory";
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(16, 24);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(120, 23);
            this.label1.TabIndex = 2;
            this.label1.Text = "Boot From";
            // 
            // label2
            // 
            this.label2.Location = new System.Drawing.Point(16, 96);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(120, 23);
            this.label2.TabIndex = 3;
            this.label2.Text = "SMP (1 to 255)";
            // 
            // radioButton5
            // 
            this.radioButton5.CausesValidation = false;
            this.radioButton5.Checked = true;
            this.radioButton5.Location = new System.Drawing.Point(152, 120);
            this.radioButton5.Name = "radioButton5";
            this.radioButton5.Size = new System.Drawing.Size(48, 24);
            this.radioButton5.TabIndex = 8;
            this.radioButton5.TabStop = true;
            this.radioButton5.Text = "Yes";
            this.radioButton5.CheckedChanged += new System.EventHandler(this.radioButton5_CheckedChanged);
            // 
            // label4
            // 
            this.label4.Location = new System.Drawing.Point(152, 96);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(112, 23);
            this.label4.TabIndex = 7;
            this.label4.Text = "Showing VGA Output";
            // 
            // radioButton6
            // 
            this.radioButton6.CausesValidation = false;
            this.radioButton6.Location = new System.Drawing.Point(208, 120);
            this.radioButton6.Name = "radioButton6";
            this.radioButton6.Size = new System.Drawing.Size(56, 24);
            this.radioButton6.TabIndex = 9;
            this.radioButton6.Text = "No";
            this.radioButton6.CheckedChanged += new System.EventHandler(this.radioButton6_CheckedChanged);
            // 
            // textBox3
            // 
            this.textBox3.Location = new System.Drawing.Point(152, 56);
            this.textBox3.Name = "textBox3";
            this.textBox3.Size = new System.Drawing.Size(112, 20);
            this.textBox3.TabIndex = 5;
            this.textBox3.Text = "32";
            // 
            // comboBox1
            // 
            this.comboBox1.DisplayMember = "1";
            this.comboBox1.Items.AddRange(new object[] {
                                                           "Floppy",
                                                           "HardDisk",
                                                           "CDRom"});
            this.comboBox1.Location = new System.Drawing.Point(16, 56);
            this.comboBox1.Name = "comboBox1";
            this.comboBox1.Size = new System.Drawing.Size(121, 21);
            this.comboBox1.TabIndex = 1;
            this.comboBox1.Text = "HardDisk";
            // 
            // groupBox10
            // 
            this.groupBox10.Controls.Add(this.radioButton7);
            this.groupBox10.Controls.Add(this.radioButton8);
            this.groupBox10.Location = new System.Drawing.Point(328, 8);
            this.groupBox10.Name = "groupBox10";
            this.groupBox10.Size = new System.Drawing.Size(120, 56);
            this.groupBox10.TabIndex = 18;
            this.groupBox10.TabStop = false;
            this.groupBox10.Text = "Set clock";
            // 
            // radioButton7
            // 
            this.radioButton7.CausesValidation = false;
            this.radioButton7.Checked = true;
            this.radioButton7.Location = new System.Drawing.Point(8, 16);
            this.radioButton7.Name = "radioButton7";
            this.radioButton7.Size = new System.Drawing.Size(48, 24);
            this.radioButton7.TabIndex = 10;
            this.radioButton7.TabStop = true;
            this.radioButton7.Text = "Yes";
            // 
            // radioButton8
            // 
            this.radioButton8.CausesValidation = false;
            this.radioButton8.Location = new System.Drawing.Point(64, 16);
            this.radioButton8.Name = "radioButton8";
            this.radioButton8.Size = new System.Drawing.Size(40, 24);
            this.radioButton8.TabIndex = 11;
            this.radioButton8.Text = "No";
            // 
            // groupBox11
            // 
            this.groupBox11.Controls.Add(this.radioButton9);
            this.groupBox11.Controls.Add(this.radioButton10);
            this.groupBox11.Location = new System.Drawing.Point(328, 80);
            this.groupBox11.Name = "groupBox11";
            this.groupBox11.Size = new System.Drawing.Size(120, 64);
            this.groupBox11.TabIndex = 19;
            this.groupBox11.TabStop = false;
            this.groupBox11.Text = "Start in fullscreen";
            // 
            // radioButton9
            // 
            this.radioButton9.CausesValidation = false;
            this.radioButton9.Location = new System.Drawing.Point(8, 24);
            this.radioButton9.Name = "radioButton9";
            this.radioButton9.Size = new System.Drawing.Size(48, 24);
            this.radioButton9.TabIndex = 13;
            this.radioButton9.Text = "Yes";
            // 
            // radioButton10
            // 
            this.radioButton10.CausesValidation = false;
            this.radioButton10.Checked = true;
            this.radioButton10.Location = new System.Drawing.Point(56, 24);
            this.radioButton10.Name = "radioButton10";
            this.radioButton10.Size = new System.Drawing.Size(56, 24);
            this.radioButton10.TabIndex = 14;
            this.radioButton10.TabStop = true;
            this.radioButton10.Text = "No";
            // 
            // tabPage4
            // 
            this.tabPage4.Controls.Add(this.groupBox4);
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Size = new System.Drawing.Size(480, 222);
            this.tabPage4.TabIndex = 4;
            this.tabPage4.Text = "CDRom";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.textBox1);
            this.groupBox4.Controls.Add(this.radioButton3);
            this.groupBox4.Controls.Add(this.button8);
            this.groupBox4.Controls.Add(this.checkBox8);
            this.groupBox4.Controls.Add(this.radioButton4);
            this.groupBox4.Location = new System.Drawing.Point(168, 24);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(176, 176);
            this.groupBox4.TabIndex = 5;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "CDROM";
            // 
            // textBox1
            // 
            this.textBox1.Enabled = false;
            this.textBox1.Location = new System.Drawing.Point(16, 72);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(152, 20);
            this.textBox1.TabIndex = 5;
            this.textBox1.Text = "HOST_CDROM_Letter_Name";
            // 
            // radioButton3
            // 
            this.radioButton3.Checked = true;
            this.radioButton3.Enabled = false;
            this.radioButton3.Location = new System.Drawing.Point(16, 48);
            this.radioButton3.Name = "radioButton3";
            this.radioButton3.TabIndex = 3;
            this.radioButton3.TabStop = true;
            this.radioButton3.Text = "Host CDRom";
            this.radioButton3.CheckedChanged += new System.EventHandler(this.radioButton3_CheckedChanged);
            // 
            // button8
            // 
            this.button8.Enabled = false;
            this.button8.Location = new System.Drawing.Point(16, 128);
            this.button8.Name = "button8";
            this.button8.Size = new System.Drawing.Size(152, 23);
            this.button8.TabIndex = 2;
            this.button8.Text = " Browse Images File";
            this.button8.Click += new System.EventHandler(this.button8_Click);
            // 
            // checkBox8
            // 
            this.checkBox8.Location = new System.Drawing.Point(16, 24);
            this.checkBox8.Name = "checkBox8";
            this.checkBox8.Size = new System.Drawing.Size(72, 24);
            this.checkBox8.TabIndex = 0;
            this.checkBox8.Text = "CDROM";
            this.checkBox8.CheckedChanged += new System.EventHandler(this.checkBox8_CheckedChanged);
            // 
            // radioButton4
            // 
            this.radioButton4.Enabled = false;
            this.radioButton4.Location = new System.Drawing.Point(16, 104);
            this.radioButton4.Name = "radioButton4";
            this.radioButton4.Size = new System.Drawing.Size(144, 24);
            this.radioButton4.TabIndex = 4;
            this.radioButton4.Text = "Image File";
            this.radioButton4.CheckedChanged += new System.EventHandler(this.radioButton4_CheckedChanged);
            // 
            // tabPage3
            // 
            this.tabPage3.Controls.Add(this.groupBox2);
            this.tabPage3.Location = new System.Drawing.Point(4, 22);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Size = new System.Drawing.Size(480, 222);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "Floppy";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.button2);
            this.groupBox2.Controls.Add(this.button1);
            this.groupBox2.Controls.Add(this.checkBox2);
            this.groupBox2.Controls.Add(this.checkBox1);
            this.groupBox2.Location = new System.Drawing.Point(184, 40);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(128, 152);
            this.groupBox2.TabIndex = 4;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Floppy";
            // 
            // button2
            // 
            this.button2.Enabled = false;
            this.button2.Location = new System.Drawing.Point(16, 112);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(104, 23);
            this.button2.TabIndex = 3;
            this.button2.Text = "Browse Disk B";
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // button1
            // 
            this.button1.Enabled = false;
            this.button1.Location = new System.Drawing.Point(16, 48);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(104, 23);
            this.button1.TabIndex = 2;
            this.button1.Text = "Browse Disk A";
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // checkBox2
            // 
            this.checkBox2.Location = new System.Drawing.Point(16, 88);
            this.checkBox2.Name = "checkBox2";
            this.checkBox2.Size = new System.Drawing.Size(72, 24);
            this.checkBox2.TabIndex = 1;
            this.checkBox2.Text = "Floppy B";
            this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
            // 
            // checkBox1
            // 
            this.checkBox1.Location = new System.Drawing.Point(16, 24);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(72, 24);
            this.checkBox1.TabIndex = 0;
            this.checkBox1.Text = "Floppy A";
            this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
            // 
            // HardDisk
            // 
            this.HardDisk.Controls.Add(this.groupBox3);
            this.HardDisk.Location = new System.Drawing.Point(4, 22);
            this.HardDisk.Name = "HardDisk";
            this.HardDisk.Size = new System.Drawing.Size(480, 222);
            this.HardDisk.TabIndex = 3;
            this.HardDisk.Text = " Harddisk";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.button5);
            this.groupBox3.Controls.Add(this.button6);
            this.groupBox3.Controls.Add(this.checkBox5);
            this.groupBox3.Controls.Add(this.checkBox6);
            this.groupBox3.Controls.Add(this.button3);
            this.groupBox3.Controls.Add(this.button4);
            this.groupBox3.Controls.Add(this.checkBox3);
            this.groupBox3.Controls.Add(this.checkBox4);
            this.groupBox3.Location = new System.Drawing.Point(136, 40);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(232, 152);
            this.groupBox3.TabIndex = 5;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = " Harddisk";
            // 
            // button5
            // 
            this.button5.Enabled = false;
            this.button5.Location = new System.Drawing.Point(120, 112);
            this.button5.Name = "button5";
            this.button5.Size = new System.Drawing.Size(104, 23);
            this.button5.TabIndex = 11;
            this.button5.Text = " Browse Disk D";
            this.button5.Click += new System.EventHandler(this.button5_Click);
            // 
            // button6
            // 
            this.button6.Enabled = false;
            this.button6.Location = new System.Drawing.Point(120, 48);
            this.button6.Name = "button6";
            this.button6.Size = new System.Drawing.Size(104, 23);
            this.button6.TabIndex = 10;
            this.button6.Text = " Browse Disk C";
            this.button6.Click += new System.EventHandler(this.button6_Click);
            // 
            // checkBox5
            // 
            this.checkBox5.Location = new System.Drawing.Point(120, 88);
            this.checkBox5.Name = "checkBox5";
            this.checkBox5.Size = new System.Drawing.Size(72, 24);
            this.checkBox5.TabIndex = 9;
            this.checkBox5.Text = "HDD";
            this.checkBox5.CheckedChanged += new System.EventHandler(this.checkBox5_CheckedChanged);
            // 
            // checkBox6
            // 
            this.checkBox6.Location = new System.Drawing.Point(120, 24);
            this.checkBox6.Name = "checkBox6";
            this.checkBox6.Size = new System.Drawing.Size(72, 24);
            this.checkBox6.TabIndex = 8;
            this.checkBox6.Text = "HDC";
            this.checkBox6.CheckedChanged += new System.EventHandler(this.checkBox6_CheckedChanged);
            // 
            // button3
            // 
            this.button3.Enabled = false;
            this.button3.Location = new System.Drawing.Point(8, 112);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(104, 23);
            this.button3.TabIndex = 7;
            this.button3.Text = " Browse Disk B";
            this.button3.Click += new System.EventHandler(this.button3_Click);
            // 
            // button4
            // 
            this.button4.Enabled = false;
            this.button4.Location = new System.Drawing.Point(8, 48);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(104, 23);
            this.button4.TabIndex = 6;
            this.button4.Text = " Browse Disk A";
            this.button4.Click += new System.EventHandler(this.button4_Click);
            // 
            // checkBox3
            // 
            this.checkBox3.Location = new System.Drawing.Point(8, 88);
            this.checkBox3.Name = "checkBox3";
            this.checkBox3.Size = new System.Drawing.Size(72, 24);
            this.checkBox3.TabIndex = 5;
            this.checkBox3.Text = "HDB";
            this.checkBox3.CheckedChanged += new System.EventHandler(this.checkBox3_CheckedChanged);
            // 
            // checkBox4
            // 
            this.checkBox4.Location = new System.Drawing.Point(8, 24);
            this.checkBox4.Name = "checkBox4";
            this.checkBox4.Size = new System.Drawing.Size(72, 24);
            this.checkBox4.TabIndex = 4;
            this.checkBox4.Text = "HDA";
            this.checkBox4.CheckedChanged += new System.EventHandler(this.checkBox4_CheckedChanged);
            // 
            // tabPage6
            // 
            this.tabPage6.Controls.Add(this.groupBox9);
            this.tabPage6.Controls.Add(this.groupBox8);
            this.tabPage6.Location = new System.Drawing.Point(4, 22);
            this.tabPage6.Name = "tabPage6";
            this.tabPage6.Size = new System.Drawing.Size(480, 222);
            this.tabPage6.TabIndex = 6;
            this.tabPage6.Text = "Tools";
            // 
            // groupBox9
            // 
            this.groupBox9.Controls.Add(this.label11);
            this.groupBox9.Controls.Add(this.textBox4);
            this.groupBox9.Controls.Add(this.label12);
            this.groupBox9.Controls.Add(this.button13);
            this.groupBox9.Controls.Add(this.comboBox3);
            this.groupBox9.Location = new System.Drawing.Point(264, 16);
            this.groupBox9.Name = "groupBox9";
            this.groupBox9.Size = new System.Drawing.Size(160, 184);
            this.groupBox9.TabIndex = 6;
            this.groupBox9.TabStop = false;
            this.groupBox9.Text = "HardDisk Tools";
            // 
            // textBox4
            // 
            this.textBox4.Location = new System.Drawing.Point(16, 64);
            this.textBox4.Name = "textBox4";
            this.textBox4.Size = new System.Drawing.Size(64, 20);
            this.textBox4.TabIndex = 3;
            this.textBox4.Text = "512";
            // 
            // label12
            // 
            this.label12.Location = new System.Drawing.Point(16, 32);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(64, 24);
            this.label12.TabIndex = 2;
            this.label12.Text = "Image Size";
            // 
            // button13
            // 
            this.button13.Location = new System.Drawing.Point(16, 112);
            this.button13.Name = "button13";
            this.button13.Size = new System.Drawing.Size(128, 48);
            this.button13.TabIndex = 1;
            this.button13.Text = "Browse and Create";
            this.button13.Click += new System.EventHandler(this.button13_Click);
            // 
            // groupBox8
            // 
            this.groupBox8.Controls.Add(this.button12);
            this.groupBox8.Controls.Add(this.label8);
            this.groupBox8.Controls.Add(this.button10);
            this.groupBox8.Controls.Add(this.label9);
            this.groupBox8.Controls.Add(this.button11);
            this.groupBox8.Controls.Add(this.label10);
            this.groupBox8.Location = new System.Drawing.Point(56, 16);
            this.groupBox8.Name = "groupBox8";
            this.groupBox8.Size = new System.Drawing.Size(144, 184);
            this.groupBox8.TabIndex = 5;
            this.groupBox8.TabStop = false;
            this.groupBox8.Text = "VDK Tool";
            // 
            // button12
            // 
            this.button12.Enabled = false;
            this.button12.Location = new System.Drawing.Point(16, 152);
            this.button12.Name = "button12";
            this.button12.Size = new System.Drawing.Size(112, 23);
            this.button12.TabIndex = 5;
            this.button12.Text = "UnMount";
            // 
            // label8
            // 
            this.label8.Location = new System.Drawing.Point(16, 24);
            this.label8.Name = "label8";
            this.label8.TabIndex = 0;
            this.label8.Text = "Path to VDK";
            // 
            // button10
            // 
            this.button10.Location = new System.Drawing.Point(16, 48);
            this.button10.Name = "button10";
            this.button10.Size = new System.Drawing.Size(112, 23);
            this.button10.TabIndex = 1;
            this.button10.Text = " Browse";
            this.button10.Click += new System.EventHandler(this.button10_Click);
            // 
            // label9
            // 
            this.label9.Location = new System.Drawing.Point(16, 80);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(112, 23);
            this.label9.TabIndex = 2;
            this.label9.Text = "Mount a HdX Image";
            // 
            // button11
            // 
            this.button11.Enabled = false;
            this.button11.Location = new System.Drawing.Point(16, 104);
            this.button11.Name = "button11";
            this.button11.Size = new System.Drawing.Size(112, 23);
            this.button11.TabIndex = 3;
            this.button11.Text = "Mount";
            this.button11.Click += new System.EventHandler(this.button11_Click);
            // 
            // label10
            // 
            this.label10.Location = new System.Drawing.Point(24, 136);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(112, 23);
            this.label10.TabIndex = 4;
            this.label10.Text = "UnMount HdX Image";
            // 
            // tabPage5
            // 
            this.tabPage5.Controls.Add(this.groupBox5);
            this.tabPage5.Location = new System.Drawing.Point(4, 22);
            this.tabPage5.Name = "tabPage5";
            this.tabPage5.Size = new System.Drawing.Size(480, 222);
            this.tabPage5.TabIndex = 5;
            this.tabPage5.Text = "Audio";
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.checkBox9);
            this.groupBox5.Controls.Add(this.checkBox7);
            this.groupBox5.Controls.Add(this.checkBox11);
            this.groupBox5.Controls.Add(this.checkBox12);
            this.groupBox5.Location = new System.Drawing.Point(8, 16);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Size = new System.Drawing.Size(192, 152);
            this.groupBox5.TabIndex = 4;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Emulate Audio Cards";
            // 
            // checkBox9
            // 
            this.checkBox9.Checked = true;
            this.checkBox9.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox9.Location = new System.Drawing.Point(8, 48);
            this.checkBox9.Name = "checkBox9";
            this.checkBox9.Size = new System.Drawing.Size(176, 32);
            this.checkBox9.TabIndex = 1;
            this.checkBox9.Text = "Creative Sound Blaster 16";
            // 
            // checkBox7
            // 
            this.checkBox7.Checked = true;
            this.checkBox7.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox7.Location = new System.Drawing.Point(8, 16);
            this.checkBox7.Name = "checkBox7";
            this.checkBox7.Size = new System.Drawing.Size(168, 24);
            this.checkBox7.TabIndex = 0;
            this.checkBox7.Text = "PC Speaker";
            // 
            // checkBox11
            // 
            this.checkBox11.Location = new System.Drawing.Point(8, 120);
            this.checkBox11.Name = "checkBox11";
            this.checkBox11.Size = new System.Drawing.Size(176, 24);
            this.checkBox11.TabIndex = 2;
            this.checkBox11.Text = "ENSONIQ AudioPCI ES1370";
            // 
            // checkBox12
            // 
            this.checkBox12.Location = new System.Drawing.Point(8, 88);
            this.checkBox12.Name = "checkBox12";
            this.checkBox12.Size = new System.Drawing.Size(176, 24);
            this.checkBox12.TabIndex = 2;
            this.checkBox12.Text = " Yamaha YM3812 (OPL2)";
            // 
            // tabPage7
            // 
            this.tabPage7.Controls.Add(this.groupBox15);
            this.tabPage7.Controls.Add(this.groupBox14);
            this.tabPage7.Controls.Add(this.groupBox12);
            this.tabPage7.Controls.Add(this.groupBox13);
            this.tabPage7.Controls.Add(this.groupBox16);
            this.tabPage7.Controls.Add(this.checkBox16);
            this.tabPage7.Location = new System.Drawing.Point(4, 22);
            this.tabPage7.Name = "tabPage7";
            this.tabPage7.Size = new System.Drawing.Size(480, 222);
            this.tabPage7.TabIndex = 8;
            this.tabPage7.Text = "Debug";
            // 
            // groupBox15
            // 
            this.groupBox15.Controls.Add(this.textBox7);
            this.groupBox15.Controls.Add(this.label14);
            this.groupBox15.Controls.Add(this.checkBox15);
            this.groupBox15.Location = new System.Drawing.Point(336, 88);
            this.groupBox15.Name = "groupBox15";
            this.groupBox15.Size = new System.Drawing.Size(112, 88);
            this.groupBox15.TabIndex = 3;
            this.groupBox15.TabStop = false;
            this.groupBox15.Text = "VNC Server";
            // 
            // textBox7
            // 
            this.textBox7.Enabled = false;
            this.textBox7.Location = new System.Drawing.Point(8, 64);
            this.textBox7.Name = "textBox7";
            this.textBox7.Size = new System.Drawing.Size(96, 20);
            this.textBox7.TabIndex = 2;
            this.textBox7.Text = "0";
            // 
            // label14
            // 
            this.label14.Location = new System.Drawing.Point(8, 40);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(88, 23);
            this.label14.TabIndex = 1;
            this.label14.Text = "Display Number";
            // 
            // checkBox15
            // 
            this.checkBox15.Location = new System.Drawing.Point(8, 16);
            this.checkBox15.Name = "checkBox15";
            this.checkBox15.Size = new System.Drawing.Size(96, 24);
            this.checkBox15.TabIndex = 0;
            this.checkBox15.Text = "Activate VNC";
            this.checkBox15.CheckedChanged += new System.EventHandler(this.checkBox15_CheckedChanged);
            // 
            // groupBox14
            // 
            this.groupBox14.Controls.Add(this.label13);
            this.groupBox14.Controls.Add(this.textBox6);
            this.groupBox14.Controls.Add(this.checkBox14);
            this.groupBox14.Location = new System.Drawing.Point(160, 8);
            this.groupBox14.Name = "groupBox14";
            this.groupBox14.Size = new System.Drawing.Size(160, 168);
            this.groupBox14.TabIndex = 2;
            this.groupBox14.TabStop = false;
            this.groupBox14.Text = "GDB";
            // 
            // label13
            // 
            this.label13.Location = new System.Drawing.Point(8, 96);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(144, 23);
            this.label13.TabIndex = 2;
            this.label13.Text = "Change connection port.";
            // 
            // textBox6
            // 
            this.textBox6.Location = new System.Drawing.Point(8, 128);
            this.textBox6.Name = "textBox6";
            this.textBox6.Size = new System.Drawing.Size(144, 20);
            this.textBox6.TabIndex = 1;
            this.textBox6.Text = "1234";
            // 
            // checkBox14
            // 
            this.checkBox14.Location = new System.Drawing.Point(8, 32);
            this.checkBox14.Name = "checkBox14";
            this.checkBox14.Size = new System.Drawing.Size(144, 24);
            this.checkBox14.TabIndex = 0;
            this.checkBox14.Text = "Wait connection to port";
            this.checkBox14.CheckedChanged += new System.EventHandler(this.checkBox14_CheckedChanged);
            // 
            // groupBox12
            // 
            this.groupBox12.Controls.Add(this.button18);
            this.groupBox12.Controls.Add(this.checkBox10);
            this.groupBox12.Location = new System.Drawing.Point(32, 8);
            this.groupBox12.Name = "groupBox12";
            this.groupBox12.Size = new System.Drawing.Size(120, 80);
            this.groupBox12.TabIndex = 1;
            this.groupBox12.TabStop = false;
            this.groupBox12.Text = "Serial Port";
            // 
            // button18
            // 
            this.button18.Enabled = false;
            this.button18.Location = new System.Drawing.Point(8, 48);
            this.button18.Name = "button18";
            this.button18.Size = new System.Drawing.Size(96, 23);
            this.button18.TabIndex = 3;
            this.button18.Text = "Browse";
            this.button18.Click += new System.EventHandler(this.button18_Click);
            // 
            // checkBox10
            // 
            this.checkBox10.Location = new System.Drawing.Point(8, 16);
            this.checkBox10.Name = "checkBox10";
            this.checkBox10.TabIndex = 2;
            this.checkBox10.Text = "Redirect to File";
            this.checkBox10.CheckedChanged += new System.EventHandler(this.checkBox10_CheckedChanged);
            // 
            // groupBox13
            // 
            this.groupBox13.Controls.Add(this.button19);
            this.groupBox13.Controls.Add(this.checkBox13);
            this.groupBox13.Location = new System.Drawing.Point(32, 96);
            this.groupBox13.Name = "groupBox13";
            this.groupBox13.Size = new System.Drawing.Size(120, 80);
            this.groupBox13.TabIndex = 1;
            this.groupBox13.TabStop = false;
            this.groupBox13.Text = "Pararell Port";
            // 
            // button19
            // 
            this.button19.Enabled = false;
            this.button19.Location = new System.Drawing.Point(8, 48);
            this.button19.Name = "button19";
            this.button19.Size = new System.Drawing.Size(96, 23);
            this.button19.TabIndex = 3;
            this.button19.Text = "Browse";
            this.button19.Click += new System.EventHandler(this.button19_Click);
            // 
            // checkBox13
            // 
            this.checkBox13.Location = new System.Drawing.Point(8, 16);
            this.checkBox13.Name = "checkBox13";
            this.checkBox13.TabIndex = 2;
            this.checkBox13.Text = "Redirect to File";
            this.checkBox13.CheckedChanged += new System.EventHandler(this.checkBox13_CheckedChanged);
            // 
            // groupBox16
            // 
            this.groupBox16.Controls.Add(this.button20);
            this.groupBox16.Location = new System.Drawing.Point(336, 8);
            this.groupBox16.Name = "groupBox16";
            this.groupBox16.Size = new System.Drawing.Size(112, 72);
            this.groupBox16.TabIndex = 1;
            this.groupBox16.TabStop = false;
            this.groupBox16.Text = "LoadVM state";
            // 
            // button20
            // 
            this.button20.Location = new System.Drawing.Point(8, 24);
            this.button20.Name = "button20";
            this.button20.Size = new System.Drawing.Size(96, 23);
            this.button20.TabIndex = 3;
            this.button20.Text = "Browse";
            this.button20.Click += new System.EventHandler(this.button20_Click);
            // 
            // checkBox16
            // 
            this.checkBox16.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
            this.checkBox16.Location = new System.Drawing.Point(32, 184);
            this.checkBox16.Name = "checkBox16";
            this.checkBox16.Size = new System.Drawing.Size(416, 32);
            this.checkBox16.TabIndex = 2;
            this.checkBox16.Text = "Simulate a standard VGA card with Bochs VBE 3.0 extensions ";
            this.checkBox16.CheckedChanged += new System.EventHandler(this.checkBox16_CheckedChanged);
            // 
            // tabPage1
            // 
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Size = new System.Drawing.Size(480, 222);
            this.tabPage1.TabIndex = 7;
            this.tabPage1.Text = "NetWork";
            // 
            // tabPage8
            // 
            this.tabPage8.Controls.Add(this.button17);
            this.tabPage8.Controls.Add(this.label7);
            this.tabPage8.Controls.Add(this.button16);
            this.tabPage8.Controls.Add(this.label6);
            this.tabPage8.Controls.Add(this.button15);
            this.tabPage8.Controls.Add(this.label5);
            this.tabPage8.Controls.Add(this.textBox5);
            this.tabPage8.Location = new System.Drawing.Point(4, 22);
            this.tabPage8.Name = "tabPage8";
            this.tabPage8.Size = new System.Drawing.Size(480, 222);
            this.tabPage8.TabIndex = 9;
            this.tabPage8.Text = "About";
            // 
            // button17
            // 
            this.button17.Location = new System.Drawing.Point(352, 160);
            this.button17.Name = "button17";
            this.button17.Size = new System.Drawing.Size(120, 56);
            this.button17.TabIndex = 6;
            this.button17.Text = "Save";
            this.button17.Click += new System.EventHandler(this.button17_Click);
            // 
            // label7
            // 
            this.label7.Location = new System.Drawing.Point(352, 136);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(120, 23);
            this.label7.TabIndex = 5;
            this.label7.Text = "Save Config";
            // 
            // button16
            // 
            this.button16.Location = new System.Drawing.Point(184, 160);
            this.button16.Name = "button16";
            this.button16.Size = new System.Drawing.Size(112, 56);
            this.button16.TabIndex = 4;
            this.button16.Text = "Load";
            this.button16.Click += new System.EventHandler(this.button16_Click);
            // 
            // label6
            // 
            this.label6.Location = new System.Drawing.Point(192, 136);
            this.label6.Name = "label6";
            this.label6.TabIndex = 3;
            this.label6.Text = "Load Config";
            // 
            // button15
            // 
            this.button15.Location = new System.Drawing.Point(8, 160);
            this.button15.Name = "button15";
            this.button15.Size = new System.Drawing.Size(112, 56);
            this.button15.TabIndex = 2;
            this.button15.Text = "Where is Qemu";
            this.button15.Click += new System.EventHandler(this.button15_Click);
            // 
            // label5
            // 
            this.label5.Location = new System.Drawing.Point(16, 136);
            this.label5.Name = "label5";
            this.label5.TabIndex = 1;
            this.label5.Text = "Qemu path";
            // 
            // textBox5
            // 
            this.textBox5.Location = new System.Drawing.Point(8, 8);
            this.textBox5.Multiline = true;
            this.textBox5.Name = "textBox5";
            this.textBox5.ReadOnly = true;
            this.textBox5.Size = new System.Drawing.Size(464, 120);
            this.textBox5.TabIndex = 0;
            this.textBox5.Text = "This program is written by Magnus Olsen for ReactOS in C# to use with Qemu and VD" +
                "K\r\n\r\n License GPL source code is in ReactOS SVN \r\n\r\nCopyRights (R) 2006 By Magnu" +
                "s Olsen \r\n(magnus@greatlord.com / greatlord@reactos.com)";
            // 
            // tabPage9
            // 
            this.tabPage9.Location = new System.Drawing.Point(4, 22);
            this.tabPage9.Name = "tabPage9";
            this.tabPage9.Size = new System.Drawing.Size(480, 222);
            this.tabPage9.TabIndex = 10;
            this.tabPage9.Text = "Config";
            // 
            // button7
            // 
            this.button7.Location = new System.Drawing.Point(24, 264);
            this.button7.Name = "button7";
            this.button7.Size = new System.Drawing.Size(488, 23);
            this.button7.TabIndex = 4;
            this.button7.Text = "Lanuch Qemu 0.8.1 (32Bits Guest)";
            this.button7.Click += new System.EventHandler(this.button7_Click);
            // 
            // openFile
            // 
            this.openFile.Title = "Path to VDK";
            this.openFile.ValidateNames = false;
            // 
            // comboBox3
            // 
            this.comboBox3.Items.AddRange(new object[] {
                                                           "cloop",
                                                           "cow ",
                                                           "qcow",
                                                           "raw ",
                                                           "vmdk ",
                                                           "",
                                                           " "});
            this.comboBox3.Location = new System.Drawing.Point(96, 64);
            this.comboBox3.Name = "comboBox3";
            this.comboBox3.Size = new System.Drawing.Size(56, 21);
            this.comboBox3.TabIndex = 7;
            this.comboBox3.Text = "vmdk ";
            // 
            // label11
            // 
            this.label11.Location = new System.Drawing.Point(96, 32);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(48, 23);
            this.label11.TabIndex = 5;
            this.label11.Text = "Format";
            // 
            // Form1
            // 
            this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
            this.ClientSize = new System.Drawing.Size(536, 293);
            this.Controls.Add(this.button7);
            this.Controls.Add(this.HardDisk2);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.ImeMode = System.Windows.Forms.ImeMode.On;
            this.Name = "Form1";
            this.Text = "Qemu GUI Lancher Version 1.0 written by Magnus Olsen for ReactOS";
            this.groupBox1.ResumeLayout(false);
            this.HardDisk2.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.groupBox7.ResumeLayout(false);
            this.groupBox6.ResumeLayout(false);
            this.groupBox10.ResumeLayout(false);
            this.groupBox11.ResumeLayout(false);
            this.tabPage4.ResumeLayout(false);
            this.groupBox4.ResumeLayout(false);
            this.tabPage3.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.HardDisk.ResumeLayout(false);
            this.groupBox3.ResumeLayout(false);
            this.tabPage6.ResumeLayout(false);
            this.groupBox9.ResumeLayout(false);
            this.groupBox8.ResumeLayout(false);
            this.tabPage5.ResumeLayout(false);
            this.groupBox5.ResumeLayout(false);
            this.tabPage7.ResumeLayout(false);
            this.groupBox15.ResumeLayout(false);
            this.groupBox14.ResumeLayout(false);
            this.groupBox12.ResumeLayout(false);
            this.groupBox13.ResumeLayout(false);
            this.groupBox16.ResumeLayout(false);
            this.tabPage8.ResumeLayout(false);
            this.ResumeLayout(false);

        }
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}

        private void groupBox1_Enter(object sender, System.EventArgs e)
        {
        
        }

        private void radioButton6_CheckedChanged(object sender, System.EventArgs e)
        {
           
           
        }

        private void radioButton5_CheckedChanged(object sender, System.EventArgs e)
        {
          
        }
        
        static string vdk_path;
        private void button10_Click(object sender, System.EventArgs e)
        {
            folderBrowserDialog1.ShowDialog();
             vdk_path = folderBrowserDialog1.SelectedPath;
            if (vdk_path != "")
            {
                button11.Enabled = true;
                button12.Enabled = true;
            }
            else
            {
                button11.Enabled = false;
                button12.Enabled = false;
            }

        }

        private void button13_Click(object sender, System.EventArgs e)
        {
            saveFileDialog1.ShowDialog();
            if (saveFileDialog1.CheckPathExists == true)
            {
                string argv; 
                long d;
                Process Qemu = new Process();
                Qemu.StartInfo.FileName   = qemu_path +"\\"+ "qemu-img.exe";    
                Qemu.StartInfo.WorkingDirectory = qemu_path ;

                d = Convert.ToInt32(textBox4.Text)*1024; //524288 kb
                argv = " create -f "+comboBox3.SelectedItem+" "+saveFileDialog1.FileName+"."+comboBox3.SelectedItem+" "+d;
                Qemu.StartInfo.Arguments = argv;
  
                try 
                {
                    Qemu.Start();
                }
                catch 
                {

                }

            }
        }

        static string qemu_path=".";
        private void button15_Click(object sender, System.EventArgs e)
        {
             folderBrowserDialog1.ShowDialog();
             qemu_path = folderBrowserDialog1.SelectedPath;
        }

        private void button11_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
        }

        private void button14_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
        }
        
        static string floppy_a;
        private void button1_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            floppy_a = openFile.FileName;
        }

        static string floppy_b;
        private void button2_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            floppy_b = openFile.FileName;            
        }

        static string hda;
        private void button4_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            hda = openFile.FileName;
        }

        static string hdc;
        private void button6_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            hdc = openFile.FileName;
        }

        static string hdb;
        private void button3_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            hdb = openFile.FileName;
        }

         static string hdd;
        private void button5_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            hdd = openFile.FileName;
        }

        private void button8_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            cdrom = openFile.FileName;
        }

        static string serial_path;
        private void button18_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            serial_path = openFile.FileName;
        }

         static string par_path;
        private void button19_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            par_path = openFile.FileName;
        }

        static string qemu_state; 
        private void button20_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            qemu_state = openFile.FileName;
        }

        private void button16_Click(object sender, System.EventArgs e)
        {
            openFile.ShowDialog();
            if (openFile.CheckFileExists == true)
            {
               loadarg(openFile.FileName);
            }   
           
         
        }

        private void loadarg(string file)
        {
            openFile.FileName = file;
            if (openFile.CheckFileExists == true)
            {
                string s_arg = "";                
                string[] split ; //= new string[70]; 
                int count =0;
                bool read = true;
                StreamReader x = new StreamReader(file);
                while (read)
                {
                    s_arg = x.ReadLine();
                    if (s_arg!="") break;
                }
              
                split = s_arg.Split(' ');                

                // rest parama
                radioButton1.Checked = true;                
                checkBox1.Checked = false;
                checkBox2.Checked = false;
                radioButton8.Checked=true;
                radioButton10.Checked=true;
                radioButton12.Checked=true;
                radioButton5.Checked=true;
                textBox3.Text = "32";
                comboBox1.Text = "HardDisk";
                comboBox2.Text = "1";
                checkBox8.Checked = false;
                checkBox3.Checked = false;
                checkBox4.Checked = false;
                checkBox5.Checked = false;
                checkBox6.Checked = false;
                textBox4.Text = "512";
                checkBox7.Checked = false;
                checkBox9.Checked = false;
                checkBox12.Checked = false;
                checkBox11.Checked=false;
                checkBox10.Checked=false;
                checkBox13.Checked=false;
                checkBox16.Checked=false;
                checkBox15.Checked=false;
                checkBox14.Checked=false;
                textBox6.Text="1234";


                // set parama
                while (count < split.Length)
                {
                    switch(split[count])
                    {
                        case "-L":
                            count++;
                            if (count < split.Length)
                            {
                                qemu_path = split[count];
                            }
                            break;

                        case "-M":
                            count++;
                            if (count < split.Length)
                            {
                                if (split[count].ToLower() == "pc")
                                {
                                    radioButton1.Checked=true;
                                    radioButton2.Checked=false;
                                }
                                else
                                {
                                    radioButton1.Checked=false;
                                    radioButton2.Checked=true;
                                }
                            }
                            else
                            {
                                radioButton1.Checked=true;
                                radioButton2.Checked=false;
                            }
                            break;

                        case "-fda":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox1.Checked = true;
                                floppy_a = split[count];
                            }
                            break;

                        case "-fdb":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox2.Checked = true;
                                floppy_a = split[count];
                            }
                            break;

                        case "-hda":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox4.Checked = true;
                                hda = split[count];
                            }
                            break;

                        case "-hdb":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox3.Checked = true;
                                hdb = split[count];
                            }
                            break;
                        case "-hdc":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox6.Checked = true;
                                hdb = split[count];
                            }
                            break;
                        case "-hdd":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox5.Checked = true;
                                hdb = split[count];
                            }
                            break;
                            
                        case "-cdrom":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox8.Checked = true;
                                cdrom = split[count];
                                if (cdrom.Length<4)
                                {
                                    radioButton3.Checked=true;
                                    textBox1.Text = cdrom;
                                }
                                else
                                {
                                    radioButton4.Checked = true;
                                }
                            }
                            break;

                        case "-boot":
                            count++;
                            if (count < split.Length)
                            {
                                if (split[count].ToLower() == "a")
                                {
                                    comboBox1.Text = "Floppy";
                                }
                                if (split[count].ToLower() == "c")
                                {
                                    comboBox1.Text = "HardDisk";
                                }
                                if (split[count].ToLower() == "d")
                                {
                                    comboBox1.Text = "CDRom";
                                }                                
                            }
                            break;

                        case "-m":
                            count++;
                            if (count < split.Length)
                            {
                                textBox3.Text = split[count];
                            }
                            break;

                            
                        case "-smp":
                            count++;
                            if (count < split.Length)
                            {
                                comboBox2.Text = split[count];
                            }
                            break;

                        case "-nographic":
                            count++;
                            if (count < split.Length)
                            {
                                radioButton6.Checked = true;
                            }
                            break;

                        case "-vnc":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox15.Checked = true;
                                textBox7.Text = split[count];
                            }
                            break;

                        case "-soundhw":
                            break;

                        case "-localtime":                                                        
                            radioButton7.Checked = true;                                                          
                            break;

                        case "-full-screen":
                            radioButton9.Checked = true;
                            break;

                        case "-serial":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox10.Checked = true;
                                serial_path = split[count];
                            }
                            break;

                        case "-parallel":
                            count++;
                            if (count < split.Length)
                            {
                                checkBox13.Checked = true;
                                par_path = split[count];
                            }
                            break;
                        case "-s":
                            checkBox14.Checked=true;
                            break;

                        case "-p":
                            count++;
                            if (count < split.Length)
                            {
                                textBox6.Text = split[count];
                            }       
                            break;

                        case "-std-vga":
                            checkBox16.Checked = true;
                            break;

                        case "-loadvm":
                            count++;
                            if (count < split.Length)
                            {
                                qemu_state = split[count];
                            }                            
                            break;
                        
                        default: 

                            // `-snapshot' 
                            // -Set virtual RAM
                            // `-k language
                            // -audio-help
                            // -pidfile file
                            // -win2k-hack
                            break;
                    }
                    count++;
                }
            }
    }



        private void checkBox1_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox1.Checked == true)
            {
                button1.Enabled = true;
            }
            else
            {
                button1.Enabled = false;
            }
        }

        private void checkBox2_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox2.Checked == true)
            {
                button2.Enabled = true;
            }
            else
            {
                button2.Enabled = false;
            }
        }

        private void checkBox4_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox4.Checked == true)
                button4.Enabled=true;
            else
                button4.Enabled=false;

        }

        private void checkBox6_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox6.Checked == true)
            {
                radioButton3.Enabled = false;
                textBox1.Enabled = false;
                radioButton4.Enabled = false;
                button8.Enabled = false;
                checkBox8.Enabled = false;
            }
            else
            {             
                checkBox8.Enabled = true;
            }

            if (checkBox6.Checked == true)
                button6.Enabled=true;
            else
                button6.Enabled=false;
        }

        private void checkBox3_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox3.Checked == true)
                button3.Enabled=true;
            else
                button3.Enabled=false;
        }

        private void checkBox5_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox5.Checked == true)
                button5.Enabled=true;
            else
                button5.Enabled=false;
        }

        private void checkBox8_CheckedChanged(object sender, System.EventArgs e)
        {
           
            if (checkBox6.Checked == true)
            {
                radioButton3.Enabled = false;
                textBox1.Enabled = false;
                radioButton4.Enabled = false;
                button8.Enabled = false;
                checkBox8.Enabled = false;
            }
            else
            {             
                checkBox8.Enabled = true;
            }

            if (checkBox8.Checked == true)
            {
                checkBox6.Enabled = false;

                radioButton3.Enabled = true;                
                radioButton4.Enabled = true;
                if (radioButton4.Checked == true)
                {
                    textBox1.Enabled = false;
                    button8.Enabled = true;
                }
                else
                {
                    textBox1.Enabled = true;
                    button8.Enabled = false;
                }



            }
            else
            {
                radioButton3.Enabled = false;
                textBox1.Enabled = false;
                radioButton4.Enabled = false;
                button8.Enabled = false;
                checkBox6.Enabled = true;
            }
        }

        static string cdrom;
        private void radioButton3_CheckedChanged(object sender, System.EventArgs e)
        {
            if (radioButton4.Checked == true)
            {
                textBox1.Enabled = false;
                button8.Enabled = true;
            }
            else
            {
                textBox1.Enabled = true;
                button8.Enabled = false;
            }
        }

        private void radioButton4_CheckedChanged(object sender, System.EventArgs e)
        {
            if (radioButton4.Checked == true)
            {
                textBox1.Enabled = false;
                button8.Enabled = true;
            }
            else
            {
                textBox1.Enabled = true;
                button8.Enabled = false;
            }
        }

        private void button7_Click(object sender, System.EventArgs e)
        {
            string argv; 
            Process Qemu = new Process();
            Qemu.StartInfo.FileName   = qemu_path +"\\"+ "qemu.exe";    
            Qemu.StartInfo.WorkingDirectory = qemu_path ;
            argv = GetArgv();
            Qemu.StartInfo.Arguments = argv;

            try 
            {
                Qemu.Start();
            }
            catch 
            {

            }
            
            
            }
        private string GetArgv()
        {
            string arg = "-L "+qemu_path+" ";
            bool audio_on = false;
            bool hdd_on = false;

            /// Machine settings
            if (radioButton1.Checked == false)
                arg = arg + "-M isapc ";
            else
                arg = arg + "-M pc ";

            /// Floppy settings
            if (checkBox1.Checked == true)
            {
                if (floppy_a != "" && floppy_a != null)
                {
                  arg = arg + "-fda "+floppy_a+" ";
                }
            }
            if (checkBox2.Checked == true)
            {
                if (floppy_b != "" && floppy_b != null)
                {
                    arg = arg + "-fdb "+floppy_b+" ";
                }
            }

            /// Harddisk settings
            if (checkBox4.Checked == true)
            {
                if (hda != "" && hda != null)
                {                    
                    arg = arg + "-hda "+hda+" ";
                }
            }
            if (checkBox3.Checked == true)
            {
                if (hda != "" && hdb != null)
                {                    
                    arg = arg + "-hdb "+hdb+" ";
                }
            }
            if (checkBox6.Checked == true)
            {
                if (hda != "" && hdc != null)
                {
                    hdd_on = true;
                    arg = arg + "-hdc "+hdc+" ";
                }
            }
            if (checkBox5.Checked == true)
            {
                if (hda != "" && hdd != null)
                {                    
                    arg = arg + "-hdd "+hdd+" ";
                }
            }
            /// cdrom
            if ((checkBox8.Checked == true) && (hdd_on == false))
            {
                if (radioButton3.Checked == true)
                {
                    arg = arg + "-cdrom " + textBox1.Text +" "; 
                }
                else if (cdrom != "" && cdrom != null)
                {                    
                    arg = arg + "-cdrom "+cdrom+" ";
                }
            }
            
            /// boot options
            if (comboBox1.Text == "Floppy")
            {
                arg = arg + "-boot a ";
            }            
            else if (comboBox1.Text == "HardDisk")
            {
                arg = arg + "-boot c ";
            }
           
            else if (comboBox1.Text == "CDRom")
            {
                arg = arg + "-boot d ";
            }

            /// memmory setting
            arg = arg + "-m "+textBox3.Text + " ";

            // smp setting
            arg = arg + "-smp " + comboBox2.SelectedItem + " ";
            
            // no vga output
            if (radioButton6.Checked == true)
            {
                arg = arg + "-nographic "; 
            }

            // set clock
            if (radioButton7.Checked == true)
            {
                arg = arg + "-localtime "; 
            }

            /// fullscreen ??
            if (radioButton9.Checked == true)
            {
                arg = arg + "-full-screen "; 
            }

            if (radioButton12.Checked == true)
            {
                arg = arg + "-no-kqemu "; 
            }

            /// Audio setting
            if (checkBox7.Checked == true)
            {
               audio_on = true;
               arg = arg + "-soundhw pcspk"; 
            }

            if (checkBox9.Checked == true)
            {
                if (audio_on == false)
                {
                    arg = arg + "-soundhw sb16"; 
                    audio_on = true;
                }
                else
                {
                  arg = arg + ",sb16"; 
                }
            }

            if (checkBox12.Checked == true)
            {
                if (audio_on == false)
                {
                    arg = arg + "-soundhw adlib"; 
                    audio_on = true;
                }
                else
                {
                    arg = arg + ",adlib"; 
                }
            }

            if (checkBox11.Checked == true)
            {
                if (audio_on == false)
                {
                    arg = arg + "-soundhw es1370"; 
                    audio_on = true;
                }
                else
                {
                    arg = arg + ",es1370"; 
                }
            }

            if (audio_on == true)
            {
                   arg = arg + " "; 
            }

            /// serial 
            if (checkBox10.Checked == true)
            {
                if (serial_path != "" && serial_path != null)
                    arg = arg + "-serial file:" + serial_path +" "; 
            }
            
            // paraell port
            if (checkBox13.Checked == true)
            {
                if (par_path != "" && par_path != null)
                    arg = arg + "-parallel  file:" + par_path +" "; 
            }

            //  vga standard
            if (checkBox16.Checked == true)
            {                
                  arg = arg + "-std-vga ";
            }

            // gdb
            if (checkBox14.Checked == true)
            {                
                arg = arg + "-s ";
            }

            if (textBox6.Text != "1234")
            {
                arg = arg + "-p "+textBox6.Text+" ";
            }

            // qemu state
            openFile.FileName = qemu_state;
            if (openFile.CheckFileExists == true)
            {
              arg = arg + "-loadvm "+qemu_state+" "; 
            }
            


            


            

            

            
            

            




            
            

            return arg;
        }

        private void HardDisk2_SelectedIndexChanged(object sender, System.EventArgs e)
        {
        
        }

        private void checkBox10_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox10.Checked == true)
            {
                button18.Enabled = true;
            }
            else
            {
                button18.Enabled = false;
            }
        }

        private void checkBox13_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox13.Checked == true)
            {
                button19.Enabled = true;
            }
            else
            {
                button19.Enabled = false;
            }
        
        }

        private void checkBox15_CheckedChanged(object sender, System.EventArgs e)
        {
            if (checkBox15.Checked == true)
            {
                textBox7.Enabled = true;
            }
            else
            {
                textBox7.Enabled = false;
            }
        }

        private void checkBox16_CheckedChanged(object sender, System.EventArgs e)
        {           
        }

        private void checkBox17_CheckedChanged(object sender, System.EventArgs e)
        {
         
        }

        private void checkBox14_CheckedChanged(object sender, System.EventArgs e)
        {
           
        }

        private void button17_Click(object sender, System.EventArgs e)
        {
            saveFileDialog1.ShowDialog();
                
            if ((saveFileDialog1.FileName != "") && (saveFileDialog1.FileName != null))
            {
                
                //x = saveFileDialog1.OpenFile();
                StreamWriter x = new StreamWriter(saveFileDialog1.FileName);                
                x.WriteLine(GetArgv());
                x.Close();
            }
        }

        private void button21_Click(object sender, System.EventArgs e)
        {
        
        }
	}
}
