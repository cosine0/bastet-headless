#include "Well.hpp"
#include "BastetBlockChooser.hpp"
#include "Ui.hpp"

using namespace Bastet;
using namespace std;

int main()
{
    Ui ui;
    ui.RedrawWell(new Well, BlockType::L, BlockPosition());
}
