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
#ifndef UI_H__
#define UI_H__

class UIApp: public wxApp
{
  virtual bool OnInit();
};

class UIFrame: public wxFrame
{
public:

  UIFrame(const wxString& Title, const wxPoint& Pos, const wxSize& Size);

protected:
  void OnQuit(wxCommandEvent& Event);
  void OnAbout(wxCommandEvent& Event);
  void OnInfileBrowse(wxCommandEvent& Event);
  void OnOutfileBrowse(wxCommandEvent& Event);
  void OnMIAfileBrowse(wxCommandEvent& Event);
  void OnTextEnter(wxCommandEvent& Event);
  
  void Log(std::ostringstream & OStr);

  void LoadStudents(std::ifstream & Infile);
  void InitStudents(std::vector<std::vector<std::string> > & Students);
  void LoadMissingGrades(std::ifstream & Infile);
  void CheckForC2P1NumDuplicates(std::map<std::string,
                                 std::vector<std::string> > & Students,
                                 std::set<std::string> & StudentNums);
  void DropIncompleteEntries(std::vector<std::vector<std::string> > & Students);

  std::ofstream m_Outfile;
  std::ofstream m_Logfile;
  wxTextCtrl *m_InfileTC;
  wxTextCtrl *m_OutfileTC;
  wxTextCtrl *m_MIAfileTC;
  wxTextCtrl *m_NumTC;
  wxTextCtrl *m_LogTC;
  wxButton *m_InfileButton;
  wxButton *m_OutfileButton;
  wxButton *m_MIAfileButton;
  wxStaticText *m_ResultST;
  wxStaticText *m_ColorST;
  wxStaticText *m_TotalST;
  std::streambuf *m_SBOld;

  int m_Ready;
  std::map<std::string, std::vector<std::string> > m_Students;
  std::set<std::string> m_Presences;
  std::set<std::string> m_NewcardNeeded;
  std::set<std::string> m_MissingGrades;

private:
  DECLARE_EVENT_TABLE()

};

enum
{
  ID_Quit = 1,
  ID_About,
	ID_InfileBrowse,
	ID_OutfileBrowse,
	ID_MIAfileBrowse,
	ID_StudentNum,
};

#endif
