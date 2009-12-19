// Copyright (c) 2009 ChoJin/Qualifilms.
// C2P1 Douchage Tool
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <wx/wx.h>
#include <wx/filedlg.h>
#include "ui.h"

#include "csv.h"

using namespace std;

#ifdef WIN32
  #define DIRSEP "\\"
#else
  #define DIRSEP "/"
#endif

#define INFO_C2P1NUM 0
#define INFO_LASTNAME 2
#define INFO_FIRSTNAME 3
#define INFO_STUDENTNUM 6
#define INFO_NBOFENTRIES 14

#define READY_OUT 1
#define READY_IN 2

#define C2P1DTVERSION "0.2.2"

BEGIN_EVENT_TABLE(UIFrame, wxFrame)
  EVT_MENU(ID_Quit, UIFrame::OnQuit)
  EVT_MENU(ID_About, UIFrame::OnAbout)
	EVT_BUTTON(ID_InfileBrowse,  UIFrame::OnInfileBrowse)
	EVT_BUTTON(ID_OutfileBrowse,  UIFrame::OnOutfileBrowse)
	EVT_BUTTON(ID_MIAfileBrowse,  UIFrame::OnMIAfileBrowse)
	EVT_TEXT_ENTER(ID_StudentNum, UIFrame::OnTextEnter)
END_EVENT_TABLE()

IMPLEMENT_APP(UIApp)

void ConvertWinPath(string & Path)
{
  for (unsigned i = 0; i < Path.size(); ++i)
  {
    if (Path[i] == '\\')
      Path[i] = '/';
  }
}

/********************************
   Convert a string to wx-UTF-8
*********************************/
inline wxString _U(const char String[] = "")
{
  return wxString(String, wxConvUTF8);
}

/********************************
  Log a string to both the log
  file and the textctrl
*********************************/
void UIFrame::Log(ostringstream & OStr)
{
  if (m_Logfile.is_open())
    m_Logfile << OStr.str() << flush;
  m_LogTC->AppendText(_U(OStr.str().c_str()));
}

/********************************
 Check and return the set of
 student's numbers with C2P1
 number duplicates
*********************************/
void UIFrame::CheckForC2P1NumDuplicates(map<string, vector<string> > & Students,
                                        set<string> & StudentNums)
{
  // Map from c2p1num to student info
  // (the latter is only used to retrieve information
  // for the warning messages)
  map<string, vector<string> > c2p1num_map;
  
  for (map<string, vector<string> >::const_iterator it = Students.begin();
       it != Students.end(); ++it)
  {
    if (it->second[INFO_C2P1NUM].length() == 0)
    { // WTF?! Empty c2p1 number?!
      ostringstream ostr("");
      ostr << "  Alerte : "
        << it->second[INFO_LASTNAME] << " " << it->second[INFO_FIRSTNAME]
        << " n'a pas de numéro C2P1!?!" << endl;
      Log(ostr);
    }
    else
    {
      if (c2p1num_map.find(it->second[INFO_C2P1NUM]) != c2p1num_map.end())
      {
        vector<string> & old = c2p1num_map[it->second[INFO_C2P1NUM]];
        StudentNums.insert(it->first);
        ostringstream ostr("");
        ostr << "  Alerte de doublon : "
          << it->second[INFO_LASTNAME] << " " << it->second[INFO_FIRSTNAME]
          << " (C2P1 : " << it->second[INFO_C2P1NUM] << ")"
          << " a le même numéro C2P1 que  "
          << old[INFO_LASTNAME] << " " << old[INFO_FIRSTNAME]
          << "!" << endl;
        Log(ostr);
      }
      else
        c2p1num_map[it->second[INFO_C2P1NUM]] = it->second;
    }
  }
}

/********************************
  Drop any student with
  incomplete info from the
  database
*********************************/
void UIFrame::DropIncompleteEntries(vector<vector<string> > & Students)
{
  // Walk from the end, for efficiency reasons
  for (int i = Students.size() - 1; i >= 0; --i)
  {
    if (Students[i].size() < INFO_NBOFENTRIES)
    {
      if (Students[i].size() > 0 && Students[i][INFO_C2P1NUM].length() > 0)
      {
        ostringstream ostr("");
        ostr << "  Alerte : "
          << "Informations pour C2P1 " << Students[i][INFO_C2P1NUM]
          << " incompletes, donc ignorées!" << endl;
        Log(ostr);
      }
      // Common trick for O(1) vector deletion
      Students[i] = Students.back();
      Students.pop_back();
    }
  }
}

/********************************
  Load and init students' info
  from the CSV database file
*********************************/
void UIFrame::LoadStudents(ifstream & Infile)
{
  vector<vector<string> > students;
  set<string> newcard_necessary_c2p1nums;

  ReadCSVFile(Infile, students);
  DropIncompleteEntries(students);
  InitStudents(students);
  CheckForC2P1NumDuplicates(m_Students, m_NewcardNeeded);
  {
    ostringstream ostr("");
    ostr << m_Students.size() << "/" << students.size() << " étudiants chargés." << endl;    
    Log(ostr);
  }
}

/********************************
  Init students' info to the
  internal map while performing
  a few sanity checks
*********************************/
void UIFrame::InitStudents(std::vector<vector<string> > & Students)
{
  for (unsigned i = 0; i < Students.size(); ++i)
  {
    if (Students[i][INFO_STUDENTNUM].length() != 0)
    {
      if (m_Students.find(Students[i][INFO_STUDENTNUM]) != m_Students.end())
      {
        ostringstream ostr("");
        ostr << "  Alerte de conflit : le numéro étudiant "
          << Students[i][INFO_STUDENTNUM]
          << " (C2P1 : " << Students[i][INFO_C2P1NUM]
          << ", " << Students[i][INFO_LASTNAME] << " "
          << Students[i][INFO_FIRSTNAME] << ")"
          << " est déjà présent (conflit avec C2P1 : "
          << m_Students[Students[i][INFO_STUDENTNUM]][INFO_C2P1NUM]
          << ")" << endl;
        Log(ostr);
      }
      else if (Students[i][INFO_STUDENTNUM][0] != '5')
      {
        ostringstream ostr("");
        ostr << "  Alerte : le numéro étudiant " << Students[i][INFO_STUDENTNUM]
          << " (C2P1 : " << Students[i][INFO_C2P1NUM]
          << ", " << Students[i][INFO_LASTNAME]
          << " " << Students[i][INFO_FIRSTNAME]
          << ")"
          << " ne commence pas par 05!" << endl;
        Log(ostr);
      }
      else
      {
        // The student number is "0" prefixed on the bar code
        // so let's just add it on the fly...
        string num = "0" + Students[i][INFO_STUDENTNUM];
        m_Students[num] = Students[i];
      }
    }
    else
    {
      ostringstream ostr("");
      ostr << "  Alerte : pas de numéro étudiant pour "
        << Students[i][INFO_LASTNAME] << " " << Students[i][INFO_FIRSTNAME]
        << " (C2P1 : " << Students[i][INFO_C2P1NUM] << ")" << endl;
      Log(ostr);
    }
  }
}

/********************************
  Load the file with the set
  of the student's numbers whose
  a grade is missing
*********************************/
void UIFrame::LoadMissingGrades(ifstream & Infile)
{
  vector<vector<string> > students;

  ReadCSVFile(Infile, students);
  for (unsigned i = 0; i < students.size(); ++i)
  {
    for (unsigned j = 0; j < students[i].size(); ++j)
    {
      if (students[i][j][0] == '0' && students[i][j][1] == '5')
        m_MissingGrades.insert(students[i][j]);
    }
  }
}

bool UIApp::OnInit()
{
  UIFrame *frame =
    new UIFrame(_T("C2P1DT : C2P1 Douchage Tool ;) v"C2P1DTVERSION" © ChoJin/Qualifilms"),
                wxPoint(50,50), wxSize(1200,500));
  frame->Show(true);
  SetTopWindow(frame);
  return true;
}

UIFrame::UIFrame(const wxString& Title, const wxPoint& Pos, const wxSize& Size)
  : wxFrame((wxFrame *)NULL, -1, Title, Pos, Size),
  m_InfileTC(NULL),
  m_OutfileTC(NULL),
  m_NumTC(NULL),
  m_LogTC(NULL),
  m_InfileButton(NULL),
  m_OutfileButton(NULL),
  m_ResultST(NULL),
  m_ColorST(NULL),
  m_TotalST(NULL),
  m_SBOld(NULL),
  m_Ready(0)
{
  // Menu
  wxMenu *menuFile = new wxMenu;

  menuFile->Append(ID_About, _T("&About..."));
  menuFile->AppendSeparator();
  menuFile->Append(ID_Quit, _T("E&xit"));
  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append(menuFile, _T("&File"));

  SetMenuBar(menuBar);

  // Status bar
  CreateStatusBar();
  SetStatusText(_T("Welcome to C2P1DT!"));

  // Panels
  wxPanel *panel = new wxPanel(this, -1);
  wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);

  // Output file
  wxBoxSizer *outfile_hbox = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *outfile_st =  new wxStaticText(panel, wxID_ANY, wxT("Fichier douchage : "));
  outfile_hbox->Add(outfile_st, 0, wxRIGHT, 30);
  m_OutfileTC = new wxTextCtrl(panel, wxID_ANY);
  m_OutfileTC->SetEditable(false);
  m_OutfileTC->Enable(false);
  outfile_hbox->Add(m_OutfileTC, 1);
  m_OutfileButton = new wxButton(panel, ID_OutfileBrowse, wxT("Parcourir"));
  outfile_hbox->Add(m_OutfileButton, 0, wxLEFT, 10);
  vbox->Add(outfile_hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

  // Input file
  wxBoxSizer *infile_hbox = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *infile_st =  new wxStaticText(panel, wxID_ANY, wxT("Fichier des Adhérents : "));
  infile_hbox->Add(infile_st, 0, wxRIGHT, 8);
  m_InfileTC = new wxTextCtrl(panel, wxID_ANY);
  m_InfileTC->SetEditable(false);
  m_InfileTC->Enable(false);
  infile_hbox->Add(m_InfileTC, 1);
  m_InfileButton = new wxButton(panel, ID_InfileBrowse, wxT("Parcourir"));
  infile_hbox->Add(m_InfileButton, 0, wxLEFT, 10);
  vbox->Add(infile_hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

  // MIA file
  wxBoxSizer *miafile_hbox = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *miafile_st =  new wxStaticText(panel, wxID_ANY, wxT("Fichier des notes non rendues : "));
  miafile_hbox->Add(miafile_st, 0, wxRIGHT, 8);
  m_MIAfileTC = new wxTextCtrl(panel, wxID_ANY);
  m_MIAfileTC->SetEditable(false);
  m_MIAfileTC->Enable(false);
  miafile_hbox->Add(m_MIAfileTC, 1);
  m_MIAfileButton = new wxButton(panel, ID_MIAfileBrowse, wxT("Parcourir"));
  miafile_hbox->Add(m_MIAfileButton, 0, wxLEFT, 10);
  vbox->Add(miafile_hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

  vbox->Add(-1, 10);

  // Student number
  wxBoxSizer *num_hbox = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *num_st =  new wxStaticText(panel, wxID_ANY, wxT("Numéro étudiant : "));
  num_hbox->Add(num_st, 0, wxRIGHT, 10);
  m_NumTC = new wxTextCtrl(panel, ID_StudentNum, wxT(""), wxPoint(-1, -1), wxSize(-1, -1), wxTE_PROCESS_ENTER);
  num_hbox->Add(m_NumTC , 1, wxRIGHT, 10);
  vbox->Add(num_hbox, 0, wxLEFT | wxTOP, 10);

  vbox->Add(-1, 10);

  // User entry
  wxBoxSizer *user_entry_h = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *user_entry_v = new wxBoxSizer(wxVERTICAL);

  m_ColorST =  new wxStaticText(panel, wxID_ANY, wxT("•"));
  m_ColorST->SetForegroundColour(*wxRED);
  wxFont font = m_ColorST->GetFont();
  font.SetPointSize(font.GetPointSize() * 4);
  m_ColorST->SetFont(font);

  wxBoxSizer *result_hbox = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *result_st =  new wxStaticText(panel, wxID_ANY, wxT("Résultat : "));
  result_hbox->Add(result_st, 0, wxRIGHT, 10);
  m_ResultST = new wxStaticText(panel, wxID_ANY, wxT("N/A"));
  result_hbox->Add(m_ResultST , 1, wxRIGHT, 10);

  wxBoxSizer *total_hbox = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *total_st =  new wxStaticText(panel, wxID_ANY, wxT("Total : "));
  total_hbox->Add(total_st, 0, wxRIGHT, 10);
  m_TotalST = new wxStaticText(panel, wxID_ANY, wxT("0"));
  total_hbox->Add(m_TotalST , 1, wxRIGHT, 10);
    
  user_entry_h->Add(m_ColorST, 0, wxRIGHT, 10);  
  user_entry_v->Add(result_hbox, 0, wxLEFT | wxTOP, 10);
  user_entry_v->Add(total_hbox, 0, wxLEFT | wxTOP, 10);
  user_entry_h->Add(user_entry_v, 0, wxRIGHT, 10);
  vbox->Add(user_entry_h, 0, wxLEFT | wxTOP, 10);
  
  vbox->Add(-1, 20);

  // Log
  wxBoxSizer *log_hbox1 = new wxBoxSizer(wxHORIZONTAL);
  wxStaticText *log_st = new wxStaticText(panel, wxID_ANY, wxT("Log: "));
  log_hbox1->Add(log_st, 0);
  vbox->Add(log_hbox1, 0, wxLEFT | wxTOP, 10);
  vbox->Add(-1, 10);
  wxBoxSizer *log_hbox2 = new wxBoxSizer(wxHORIZONTAL);
  m_LogTC = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxPoint(-1, -1), wxSize(-1, -1), wxTE_MULTILINE | wxTE_READONLY  /* | wxTE_RICH2 */); //520905510
  m_LogTC->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour,  wxFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL)));
  log_hbox2->Add(m_LogTC, 1, wxEXPAND);
  vbox->Add(log_hbox2, 1, wxLEFT | wxRIGHT | wxEXPAND, 10);

  panel->SetSizer(vbox);
  Centre();

  // Redirect stdout to the textctrl
  // It doesn't seem to be working quite well with utf-8...
  //  m_SBOld = cout.rdbuf();
  //  cout.rdbuf(m_LogTC);	
}

void UIFrame::OnQuit(wxCommandEvent& WXUNUSED(Event))
{
  if (m_Outfile.is_open())
  {
    m_Outfile.flush();
    m_Outfile.close();
  }
  m_Students.clear();
  m_Presences.clear();
  m_NewcardNeeded.clear();
  m_MissingGrades.clear();
  Close(true);
}

void UIFrame::OnAbout(wxCommandEvent& WXUNUSED(Event))
{
  wxMessageBox(_T("C2P1DT : C2P1 Douchage Tool\nv"C2P1DTVERSION"\nCopyright © 2009 ChoJin/Qualifilms"
                "\n\n"
                "This program is free software: you can redistribute it and/or "
                "modify it under the terms of the GNU General Public License as "
                "published by the Free Software Foundation, either version 3 of "
                "the License, or (at your option) any later version.\n"
                "\n"
                "This program is distributed in the hope that it will be useful, "
                "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
                "GNU General Public License for more details.\n"
                "\n"
                "You should have received a copy of the GNU General Public License "
                "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"
                ),
               _T("About C2P1DT"), wxOK | wxICON_INFORMATION, this);
}

void UIFrame::OnInfileBrowse(wxCommandEvent& WXUNUSED(Event))
{
  wxFileDialog *f = new wxFileDialog(this, _T("Selectionnez le fichier Adhérent"),
                                     _T(""), _T(""), _T("*.csv"),
                                     wxOPEN | wxFILE_MUST_EXIST);

  if ( f->ShowModal() ==  wxID_OK )
  {
    m_Students.clear();
    m_NewcardNeeded.clear();
    ifstream infile;
    {
      ostringstream ostr("");
      ostr << "Ouverture du fichier d'adhérent '" << f->GetPath().char_str() << "'..." << endl;
      Log(ostr);
    }
    infile.open(f->GetPath().char_str(), ios_base::in | ios::binary);
    if (!infile.is_open())
    {
      m_Ready &= ~READY_IN;
      ostringstream ostr("");
      ostr << "Erreur pendant l'ouverture du fichier d'adhérent!" << endl;
      Log(ostr);
      return;
    }
    m_InfileTC->ChangeValue(f->GetPath());
    {
      ostringstream ostr("");
      ostr << "Chargement du fichier..." << endl;
      Log(ostr);
    }
    LoadStudents(infile);
    m_Ready |= READY_IN;
  }
}

void UIFrame::OnOutfileBrowse(wxCommandEvent& WXUNUSED(Event))
{
  wxFileDialog *f = new wxFileDialog(this, _T("Selectionnez le fichier de douchage"),
                                     _T(""), _T(""), _T("*.csv"),
                                     wxSAVE | wxOVERWRITE_PROMPT);

  if (f->ShowModal() ==  wxID_OK)
  {
    // Setup log file
    if (m_Logfile.is_open())
      m_Logfile.close(); 
    ostringstream ostr("");
    ostr << f->GetDirectory().char_str() << DIRSEP "log.txt";
    string path(ostr.str());
    ConvertWinPath(path);
    {
      ostringstream ostr("");
      ostr << "Création du fichier de log '" << path << "' ..." << endl;
      Log(ostr);
    }
    m_Logfile.open(path.c_str(), ios_base::out | ios::binary | ios_base::app);	
    if (!m_Logfile.is_open())
    {
      {
        ostringstream ostr("");
        ostr << "Erreur pendant la création du fichier de log!" << endl;
        Log(ostr);
      }
      m_Ready &= ~READY_OUT;
      return;
    }
    // Setup output file
    if (m_Outfile.is_open())
      m_Outfile.close();
    {
      ostringstream ostr("");
      ostr << "Création du fichier de douchage '" << f->GetPath().char_str() << "'..." << endl;
      Log(ostr);
    }
    m_Outfile.open(f->GetPath().char_str(), ios_base::out | ios::binary | ios_base::trunc);
    if (!m_Outfile.is_open())
    {
      {
        ostringstream ostr("");
        ostr << "Erreur pendant la création du fichier de douchage!" << endl;
        Log(ostr);
      }
      m_Ready &= ~READY_OUT;
      return;
    }
    m_Presences.clear();
    {
      ostringstream ostr("");
      ostr << "Présence initialisées!" << endl;
      Log(ostr);
    }
    m_OutfileTC->ChangeValue(f->GetPath()); 
    m_Ready |= READY_OUT;
  }
}

void UIFrame::OnMIAfileBrowse(wxCommandEvent& WXUNUSED(Event))
{
  wxFileDialog *f = new wxFileDialog(this, _T("Selectionnez le fichier des notes non rendues"),
                                     _T(""), _T(""), _T("*.csv"),
                                     wxOPEN | wxFILE_MUST_EXIST);

  if (f->ShowModal() ==  wxID_OK)
  {
    ifstream infile;
    {
      ostringstream ostr("");
      ostr << "Ouverture du fichier des notes manquantes" << f->GetPath().char_str() << "'..." << endl;
      Log(ostr);
    }
    infile.open(f->GetPath().char_str(), ios_base::in | ios::binary);
    if (!infile.is_open())
    {
      ostringstream ostr("");
      ostr << "Erreur pendant l'ouverture du fichier!" << endl;
      Log(ostr);
      return;
    }
    m_MIAfileTC->ChangeValue(f->GetPath());
    {
      ostringstream ostr("");
      ostr << "Chargement du fichier..." << endl;
      Log(ostr);
    }
    LoadMissingGrades(infile);
    {
      ostringstream ostr("");
      ostr << m_MissingGrades.size() << " personnes" << endl << "OK!" << endl;
      Log(ostr);
    }
  }
}

void UIFrame::OnTextEnter(wxCommandEvent& WXUNUSED(Event))
{
  if (!(m_Ready & READY_IN) || !(m_Ready & READY_OUT))
  {
    m_NumTC->ChangeValue(_T(""));
    m_ColorST->SetForegroundColour(*wxRED);
    m_ColorST->SetLabel(_U("•"));
    return;
  }

  string student_num(m_NumTC->GetValue().char_str());
  // Any student number entered?
  if (student_num.length() == 0)
  {
    m_ColorST->SetForegroundColour(*wxRED);
    m_ColorST->SetLabel(_U("•"));
    return;
  }
    
  // Check whether or not we know about this student number
  if (m_Students.find(student_num) == m_Students.end())
  {
    ostringstream ostr("");
    ostr << "Numéro étudiant " << student_num << " inconnu!" << endl;
    Log(ostr);
    m_ResultST->SetLabel(_T("Numéro inconnu!"));
    m_NumTC->ChangeValue(_T(""));
    m_ColorST->SetForegroundColour(*wxRED);
    m_ColorST->SetLabel(_U("•"));
    return;
  }
  
  // Check whether or not we already took this student into account
  if (m_Presences.find(student_num) == m_Presences.end())
  { // new one?
    m_Presences.insert(student_num);
    ostringstream ostr("");
    ostr << m_Students[student_num][INFO_LASTNAME]
         << " " << m_Students[student_num][INFO_FIRSTNAME]
         << " (C2P1 : " << m_Students[student_num][INFO_C2P1NUM] << ")" << flush;
    wxColour colour(*wxRED);

    if (m_NewcardNeeded.find(student_num) != m_NewcardNeeded.end())
    {
      ostr << " - ALERTE : Nouvelle carte C2P1 requise (doublon)!" << flush;
      colour = wxColour(0xFF, 0xA5, 00);
    }
    else if (m_MissingGrades.find(student_num) != m_MissingGrades.end())
    {
      ostr << " - ALERTE : Note non rendue!" << flush;
      colour = wxColour(0x66, 0x00, 0xFF);
    }
    else
    {
      colour = *wxGREEN;
    }
    m_ColorST->SetForegroundColour(colour);
    m_ColorST->SetLabel(_U("•"));
    m_ResultST->SetLabel(wxString::FromAscii(ostr.str().c_str()));
    m_LogTC->AppendText(wxString::FromAscii((ostr.str() + "\n").c_str()));
    m_Logfile << ostr.str() << endl << flush;
    string csvline = ToCSVLine(m_Students[student_num]);
    m_Outfile << csvline << endl << flush;
  }
  else
  { // Already seen
    m_ResultST->SetLabel(_T("Numéro déjà douché!"));
    // Don't change the color i.e. we keep the color code from
    // the latest operation
  }

  { // Update the total number
    ostringstream ostr("");
    ostr << m_Presences.size();
    m_TotalST->SetLabel(wxString::FromAscii(ostr.str().c_str()));
  }
  m_NumTC->ChangeValue(_T(""));
}

