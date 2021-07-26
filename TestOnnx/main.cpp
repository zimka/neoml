#include <string>
#include <NeoOnnx/NeoOnnx.h>
#include <NeoMathEngine/NeoMathEngine.h>
#include <NeoML/Random.h>
#include <iostream>

using namespace NeoML;

int main( int /* argc */, wchar_t* /* argv */[] )
{

    CArray<const char*> inputs;
    CArray<const char*> outputs;

    try{
        std::string path = R"--(model.onnx)--";
        std::cout << path << std::endl;
        IMathEngine* mathEng = CreateCpuMathEngine( 1, 0);

        CRandom random( 0x123 );

        CDnn net( random, *mathEng );

        const char * cpath = "model.onnx";

        NeoOnnx::LoadFromOnnx(cpath, net, inputs, outputs);

        delete mathEng;
    }
    catch( std::exception& exc ) {
        std::cout << "catch!" << std::endl;
        std::cout << exc.what() << std::endl;
        return 1;
    }
    catch( ... ){
        std::cout << "catch seh ... !" << std::endl;
        return 2;
    }
    std::cout << "tada" << std::endl;
    return 0;
}