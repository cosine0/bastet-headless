/*
    Bastet - tetris clone with embedded bastard block chooser
    (c) 2005-2009 Federico Poloni <f.polonithirtyseven@sns.it> minus 37

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Ui.hpp"
#include "BastetBlockChooser.hpp"
#include "JsonSocket.h"

#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <fstream>

using namespace std;
using namespace boost;

namespace Bastet
{

    Score &operator+=(Score &a, const Score &b)
    {
        a.first += b.first;
        a.second += b.second;
        return a;
    }

    Ui::Ui() :
            _level(0),
            _socket(nullptr), _speed(0)
    {
        /* Set random seed. */
        SetSeed(time(NULL) + 37);
    }

    Dot BoundingRect(const std::string &message)
    { //returns x and y of the minimal rectangle containing the given string
        vector<string> splits;
        split(splits, message, is_any_of("\n"));
        size_t maxlen = 0;
        BOOST_FOREACH(string &s, splits)
        {
            maxlen = max(maxlen, s.size());
        }
        return (Dot) {int(maxlen + 1), int(splits.size())};
    }

    void Ui::MessageDialog(const std::string &message)
    {
        cout << message << endl;
        int ch;
        do
        {
            ch = GetKey();
        } while (ch != ' ' && ch != 13); //13=return key!=KEY_ENTER, it seems
    }

    void Ui::MessageDialogNoWait(const std::string &message)
    {
        cout << message << endl;
    }

    std::string Ui::InputDialog(const std::string &message)
    {
        GetKey();
        return "socket player";
    }

    int Ui::KeyDialog(const std::string &message)
    {
        cout << message << endl;
        return GetKey();
    }

    size_t Ui::MenuDialog(const vector<string> &choices)
    {
        for (size_t i = 0; i < choices.size(); ++i)
        {
            cout << i << ". " << choices[i] << endl;
        }
        size_t chosen = 0;
        int ch;
        bool done = false;
        do
        {
            ch = GetKey();
            switch (ch)
            {
                case KEY_UP:
                    if (chosen == 0) break;
                    chosen--;
                    break;
                case KEY_DOWN:
                    if (chosen == choices.size() - 1) break;
                    chosen++;
                    break;
                case 13: //ENTER
                case ' ':
                    done = true;
                    break;
            }
        } while (!done);
        return chosen;
    }

    //must be <1E+06, because it should fit into a timeval usec field(see man select)
    static const boost::array<int, 10> delay = {{999999, 770000, 593000, 457000, 352000, 271000, 208000, 160000, 124000, 95000}};

    void Ui::DropBlock(BlockType b, Well *w)
    {

        if (_socket != nullptr)
        {
            rapidjson::Document doc;
            auto& allocator = doc.GetAllocator();

            doc.SetObject();
            doc.AddMember("type", "current_block", allocator);
            doc.AddMember("block", b, allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            _socket->send(buffer.GetString());
        }

        fd_set in, tmp_in;
        struct timeval time;

        FD_ZERO(&in);
        FD_SET(0, &in); //adds stdin
        if (_socket != nullptr)
           FD_SET(_socket->getFd(), &in); //adds socket

        time.tv_sec = 0;
        time.tv_usec = 100000 >> _speed;

        //assumes nodelay(stdscr,TRUE) has already been called
        BlockPosition p;

        RedrawWell(w, b, p);
        Keys *keys = config.GetKeys();

        bool key_pressed = false;
        int auto_drop_time = 0;
        while (1)
        { //break = tetromino locked
            tmp_in = in;
            int sel_ret = select(FD_SETSIZE, &tmp_in, NULL, NULL, &time);
            if (!key_pressed)
            { //keypress
                int ch;
                ch = GetKey();
                key_pressed = true;

                if (ch == keys->Left) {
                    if (_socket == nullptr)
                        _move_log.push_back(Left);
                    p.MoveIfPossible(Left, b, w);
                } else if (ch == keys->Right) {
                    if (_socket == nullptr)
                        _move_log.push_back(Right);
                    p.MoveIfPossible(Right, b, w);
                } else if (ch == keys->Down)
                {
                    if (_socket == nullptr)
                        _move_log.push_back(Down);
                    bool val = p.MoveIfPossible(Down, b, w);
                    if (val)
                    {
                        auto_drop_time = 0;
                    } else break;

                } else if (ch == keys->RotateCW) {
                    if (_socket == nullptr)
                        _move_log.push_back(RotateCW);
                    p.MoveIfPossible(RotateCW, b, w);

                } else if (ch == keys->RotateCCW) {
                    if (_socket == nullptr)
                        _move_log.push_back(RotateCCW);
                    p.MoveIfPossible(RotateCCW, b, w);
                } else if (ch == keys->Drop)
                {
                    if (_socket == nullptr)
                        _move_log.push_back(Drop);
                    p.Drop(b, w);
                    //_points+=2*fb.HardDrop(w);
                    //RedrawScore();
                    break;
                } else if (ch == keys->Pause)
                {
                    MessageDialog("Press SPACE or ENTER to resume the game");
                    RedrawWell(w, b, p);
                } else { //default...
                    if (_socket == nullptr)
                        _move_log.push_back(None);
                }
                RedrawWell(w, b, p);
            } else if (sel_ret == 0)
            {
                key_pressed = false;
                if (auto_drop_time >= delay[_level])
                {
                    // auto drop
                    if (!p.MoveIfPossible(Down, b, w))
                        break;
                    auto_drop_time = 0;
                    RedrawWell(w, b, p);
                    continue;
                }

                auto_drop_time += 100000;
                time.tv_sec = 0;
                time.tv_usec = 100000 >> _speed;
            }
            else
            {
                GetKey();
            }//keypress switch
        } //while(1)

        LinesCompleted lc = w->Lock(b, p);
        //locks also into _colors

        RedrawWell(w, b, p);
        if (lc._completed.any())
        {
            w->ClearLines(lc);

            int newlines = lc._completed.count();
            if (((_lines + newlines) / 10 - _lines / 10 != 0) && _level < 9)
            {
                _level++;
            }

            _lines += newlines;
            switch (newlines)
            {
                case 1:
                    _points += 100;
                    break;
                case 2:
                    _points += 300;
                    break;
                case 3:
                    _points += 500;
                    break;
                case 4:
                    _points += 800;
                    break;
            }
            RedrawScore();
        }
    }

    void Ui::RedrawWell(const Well *w, BlockType b, const BlockPosition &p)
    {
        if (_socket != nullptr)
        {
            string serialized_well((WellWidth + 1) * WellHeight, '0');

            const auto &cells = w->GetWell();
            for (int j = 0; j < WellHeight; ++j)
            {
                for (int i = 0; i < WellWidth; ++i)
                {
                    if (cells[j + 2][i])
                    {
                        serialized_well[(WellWidth + 1) * j + i] = '1';
                    }
                }
                serialized_well[(WellWidth + 1) * j + WellWidth] = '\n';
            }

            rapidjson::Document doc;
            auto& allocator = doc.GetAllocator();

            ofstream fout("out", ios::app);
            rapidjson::Value block_coords(rapidjson::kArrayType);
            BOOST_FOREACH(const Dot &d, p.GetDots(b))
                if (d.y >= 0) {
                    rapidjson::Value coord(rapidjson::kArrayType);
                    coord.PushBack(d.x, allocator);
                    coord.PushBack(d.y, allocator);
                    block_coords.PushBack(coord, allocator);
                }

            doc.SetObject();
            doc.AddMember("type", "well", allocator);
            doc.AddMember("well", rapidjson::StringRef(serialized_well.c_str()), allocator);
            doc.AddMember("block", block_coords, allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            _socket->send(buffer.GetString());
        }
    }

    void Ui::ClearNext()
    {
    }

    void Ui::RedrawNext(BlockType b)
    {

        if (_socket != nullptr)
        {
            rapidjson::Document doc;
            auto& allocator = doc.GetAllocator();

            doc.SetObject();
            doc.AddMember("type", "next_block", allocator);
            doc.AddMember("block", b, allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            _socket->send(buffer.GetString());
        }
    }

    void Ui::RedrawScore()
    {
        if (_socket != nullptr)
        {
            rapidjson::Document doc;
            auto& allocator = doc.GetAllocator();

            doc.SetObject();
            doc.AddMember("type", "score", allocator);
            doc.AddMember("points", _points, allocator);
            doc.AddMember("lines", _lines, allocator);
            doc.AddMember("level", _level, allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            _socket->send(buffer.GetString());
        }
        return;
    }

    void Ui::Play(BlockChooser *bc)
    {
        // [cosine] send keyboard settings
        if (_socket != nullptr)
        {
            rapidjson::Document doc;
            auto& allocator = doc.GetAllocator();

            doc.SetObject();
            doc.AddMember("type", "keys", allocator);

            auto keys = config.GetKeys();
            // workaround. this exploits the fact the key members include zero in the second byte.
            doc.AddMember("down", rapidjson::StringRef((char*) &keys->Down), allocator);
            doc.AddMember("drop", rapidjson::StringRef((char*) &keys->Drop), allocator);
            doc.AddMember("left", rapidjson::StringRef((char*) &keys->Left), allocator);
            doc.AddMember("pause", rapidjson::StringRef((char*) &keys->Pause), allocator);
            doc.AddMember("right", rapidjson::StringRef((char*) &keys->Right), allocator);
            doc.AddMember("rotate_counterclockwise", rapidjson::StringRef((char*) &keys->RotateCCW), allocator);
            doc.AddMember("rotate_clockwise", rapidjson::StringRef((char*) &keys->RotateCW), allocator);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            _socket->send(buffer.GetString());
        }
        // [/cosine]
        srandom(_seed);
        _level = 0;
        _points = 0;
        _lines = 0;
        _move_log.clear();
        RedrawScore();
        Well w;

        Queue q = bc->GetStartingQueue();
        if (q.size() == 1) //no block preview
            ClearNext();
        try
        {
            while (true)
            {
                if (_socket == nullptr)
                    while (GetKey() != ERR); //ignores the keys pressed during the next block calculation
                BlockType current = q.front();
                q.pop_front();
                if (!q.empty())
                    RedrawNext(q.front());
                DropBlock(current, &w);
                q.push_back(bc->GetNext(&w, q));
            }
        } catch (GameOver &go)
        {
            if (_socket == nullptr)
            {
                ofstream fout("bastet.rep", ios::app);
                fout << _seed << ' ';
                for (auto& move : _move_log)
                    fout << move;

                fout << endl;
            } else
            {
                _socket->send(R"({"type":"game_over"})");
            }
        }
        return;
    }

    void Ui::HandleHighScores(difficulty_t diff)
    {
        HighScores *hs = config.GetHighScores(diff);
        if (hs->Qualifies(_points))
        {
            string name = InputDialog(" Congratulations! You got a high score \n Please enter your name");
            hs->InsertHighScore(_points, name);
        } else
        {
            MessageDialog("You did not get into\n"
                                  "the high score list!\n"
                                  "\n"
                                  "     Try again!\n"
            );
        }
    }

    void Ui::ShowHighScores(difficulty_t diff)
    {
        HighScores *hs = config.GetHighScores(diff);
        string allscores;
        if (diff == difficulty_normal)
            allscores += "**Normal difficulty**\n";
        else if (diff == difficulty_hard)
            allscores += "**Hard difficulty**\n";
        format fmt("%-20.20s %8d\n");
        for (HighScores::reverse_iterator it = hs->rbegin(); it != hs->rend(); ++it)
        {
            allscores += str(fmt % it->Scorer % it->Score);
        }
        MessageDialog(allscores);
    }

    void Ui::CustomizeKeys()
    {
        Keys *keys = config.GetKeys();
        format fmt(
                "Press the key you wish to use for:\n\n"
                        "%=1.34s\n\n");
        keys->Down = KeyDialog(str(fmt % "move tetromino DOWN (soft-drop)"));
        keys->Left = KeyDialog(str(fmt % "move tetromino LEFT"));
        keys->Right = KeyDialog(str(fmt % "move tetromino RIGHT"));
        keys->RotateCW = KeyDialog(str(fmt % "rotate tetromino CLOCKWISE"));
        keys->RotateCCW = KeyDialog(str(fmt % "rotate tetromino COUNTERCLOCKWISE"));
        keys->Drop = KeyDialog(str(fmt % "DROP tetromino (move down as much as possible immediately)"));
        keys->Pause = KeyDialog(str(fmt % "PAUSE the game"));
    }

    int Ui::GetKey() const
    {
        if (_socket != nullptr)
        {
            _socket->send(R"({"type":"send_me_a_key"})");
            return *(int *) _socket->recv(4).c_str();
        }
    }

    void Ui::SetSocket(JsonSocket *sock)
    {
        _socket = sock;
    }

    void Ui::SetSpeed(int speed)
    {
        _speed = speed;
    }

    void Ui::SetSeed(unsigned int seed)
    {
        _seed = seed;
    }
}
