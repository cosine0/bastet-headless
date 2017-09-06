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

#ifndef UI_HPP
#define UI_HPP

#include "Well.hpp"
#include "BlockPosition.hpp"
#include "BlockChooser.hpp"
#include "Config.hpp"
#include "JsonSocket.h"

#include <string>

namespace Bastet
{
    typedef std::pair<int, int> Score; //(points, lines)
    Score &operator+=(Score &a, const Score &b);

    class Ui
    {
    public:
        Ui();
        void SetSocket(JsonSocket *sock);
        void SetSpeed(int speed);

        void MessageDialog(const std::string &message); //shows msg, ask for "space"
        std::string InputDialog(const std::string &message); //asks for a string
        int KeyDialog(const std::string &message); //asks for a single key
        size_t MenuDialog(const std::vector<std::string> &choices); //asks to choose one, returns index
        void RedrawWell(const Well *well, BlockType falling, const BlockPosition &pos);

        void ClearNext(); //clear the next block display
        void RedrawNext(BlockType next); //redraws the next block display
        void RedrawScore();

        void DropBlock(BlockType b, Well *w); //returns <score,lines>

        void Play(BlockChooser *bc);

        void HandleHighScores(difficulty_t diff); ///if needed, asks name for highscores
        void ShowHighScores(difficulty_t diff);

        void CustomizeKeys();
        int GetKey() const;

        void MessageDialogNoWait(const std::string &message);

        void SetSeed(unsigned int seed);

    private:
        //    difficulty_t _difficulty; //unused for now
        int _level;
        int _points;
        int _lines;

        JsonSocket* _socket;  // socket to remotely control the game
        int _speed;  // log2(multiplication to speed of game)
        unsigned int _seed;  // random seed
        std::vector<Movement> _move_log;  // key press log to make a replay file
    };
}

#endif //UI_HPP
