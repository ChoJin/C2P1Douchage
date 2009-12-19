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
#include <string>
#include <vector>
#include "csv.h"

using namespace std;

void ReadCSVLine(vector<string> & Row, const string& Line, char Delimiter)
{
  int linepos = 0;
  int inquotes = false;
  char c;
  int linemax = Line.length();
  string curstring("");
  Row.clear();
         
  while(Line[linepos] != 0 && linepos < linemax)
  {
    c = Line[linepos];

    if (!inquotes && curstring.length() == 0 && c == '"')
    {
      // beginquotechar
      inquotes = true;
    }
    else if (inquotes && c == '"')
    {
      // quotechar
      if ( (linepos + 1 <linemax) && (Line[linepos + 1] == '"') )
      {
        // encountered 2 double quotes in a row (resolves to 1 double quote)
        curstring.push_back(c);
        linepos++;
      }
      else
      {
        // endquotechar
        inquotes = false;
      }
    }
    else if (!inquotes && c == Delimiter)
    {
      // end of field
      Row.push_back(curstring);
      curstring = "";
    }
    else if (!inquotes && (c == '\r' || c == '\n'))
    {
      Row.push_back(curstring);
      return;
    }
    else
    {
      curstring.push_back(c);
    }
    linepos++;
  }
  Row.push_back(curstring);
}

void ReadCSVFile(ifstream & File, vector<vector<string> > & Result)
{
  vector<string> row;
  string line;

  while (getline(File, line) && File.good())
  {
    row.clear();
    ReadCSVLine(row, line, ';');
    Result.push_back(row);
  }
}

string ToCSVLine(vector<std::string> & Row)
{
  string res("");
  for (unsigned i = 0; i < Row.size(); ++i)
  {
    if (Row[i].find(';') >= Row[i].size())
      res += Row[i];
    else
    {
      res += "\"";
      res += Row[i];
      res += "\"";
    }
    res += ";";
  }
  return res;
}
