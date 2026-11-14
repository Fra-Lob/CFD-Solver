
#include <cfd/meshio/VTKReader.hpp>

int main()
{
    auto mesh = cfd::VTKReader::read("mesh.vtu");
}