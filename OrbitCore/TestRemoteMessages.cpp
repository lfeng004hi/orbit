#include "TestRemoteMessages.h"
#include "OrbitProcess.h"
#include "OrbitModule.h"
#include "OrbitFunction.h"
#include "TcpServer.h"
#include "TcpClient.h"
#include "Serialization.h"

#include <fstream>
#include <streambuf>
#include <sstream>
#include <iostream>

//-----------------------------------------------------------------------------
TestRemoteMessages& TestRemoteMessages::Get()
{
    static TestRemoteMessages instance;
    return instance;
}

//-----------------------------------------------------------------------------
TestRemoteMessages::TestRemoteMessages()
{

}

//-----------------------------------------------------------------------------
TestRemoteMessages::~TestRemoteMessages()
{
}

void TestRemoteMessages::Init()
{
    SetupMessageHandlers();
}

//-----------------------------------------------------------------------------
template <class T> std::string SerializeObject(T& a_Object)
{
    std::stringstream buffer;
    cereal::BinaryOutputArchive archive( buffer );
    archive(a_Object);
    PRINT_VAR(buffer.str().size());
    return buffer.str();
}

//-----------------------------------------------------------------------------
template <class T> std::string SerializeObjectHumanReadable(T& a_Object)
{
    std::stringstream ss;
    {
        cereal::XMLOutputArchive archive(ss);
        archive(a_Object);
    }
    std::cout << "ss:" << ss.str() << std::endl;
    PRINT_VAR(ss.str().size());
    return ss.str();
}

//-----------------------------------------------------------------------------
void TestRemoteMessages::Run()
{
    Process process;
    process.m_Name = "process.m_Name";
    process.m_FullName = "process.m_FullName";
    process.SetID( 22 );
    process.m_Is64Bit = true;
    process.m_DebugInfoLoaded = true;
    process.m_IsRemote = true;
    process.m_ThreadIds.insert(0);
    process.m_ThreadIds.insert(1);
    process.m_ThreadIds.insert(2);

    std::string processData = SerializeObjectHumanReadable(process);
    PRINT_VAR(processData);
    GTcpClient->Send(Msg_RemoteProcesses, (void*)processData.data(), processData.size());

    Module module;
    module.m_Name = "module.m_Name";
    module.m_FullName = "module.m_FullName";
    module.m_PdbName = "module.m_PdbName";
    module.m_Directory = "module.m_Directory";
    module.m_PrettyName = "module.m_PrettyName";
    module.m_AddressRange = "module.m_AddressRange";
    module.m_DebugSignature = "module.m_DebugSignature";
    module.m_ModuleHandle = (HMODULE)1;
    module.m_AddressStart = 2;
    module.m_AddressEnd = 3;
    module.m_EntryPoint = 4;
    module.m_FoundPdb = true;

    module.m_Selected = true;
    module.m_Loaded = true;
    module.m_PdbSize = 110;

    std::string moduleData = SerializeObjectHumanReadable(module);
    GTcpClient->Send(Msg_RemoteModules, (void*)moduleData.data(), moduleData.size());

    Function function;
    function.m_Name = "m_Name";
    function.m_PrettyName = "m_PrettyName";
    function.m_PrettyNameLower = "m_PrettyNameLower";
    function.m_Module = "m_Module";
    function.m_File = "m_File";
    function.m_Probe = "m_Probe";
    function.m_Address = 1;
    function.m_ModBase = 2;
    function.m_Size = 3;
    function.m_Id = 4;
    function.m_ParentId = 5;
    function.m_Line = 6;
    function.m_CallConv = 7;

    std::string functionData = SerializeObjectHumanReadable(function);
    GTcpClient->Send(Msg_RemoteFunctions, (void*)functionData.data(), functionData.size());
}

//-----------------------------------------------------------------------------
void TestRemoteMessages::SetupMessageHandlers()
{
    GTcpServer->SetCallback( Msg_RemoteProcesses, [=]( const Message & a_Msg )
    {
        PRINT_VAR(a_Msg.m_Size);
        std::istringstream buffer(std::string(a_Msg.m_Data, a_Msg.m_Size));
        cereal::XMLInputArchive inputAr( buffer );
        Process process;
        inputAr(process);
        PRINT_VAR(process.GetName());
    } );

    GTcpServer->SetCallback( Msg_RemoteModules, [=]( const Message & a_Msg )
    {
        PRINT_VAR(a_Msg.m_Size);
        std::istringstream buffer(std::string(a_Msg.m_Data, a_Msg.m_Size));
        cereal::XMLInputArchive inputAr( buffer );
        Module module;
        inputAr(module);
        PRINT_VAR(module.m_Name);
    } );

    GTcpServer->SetCallback( Msg_RemoteFunctions, [=]( const Message & a_Msg )
    {
        PRINT_VAR(a_Msg.m_Size);
        std::istringstream buffer(std::string(a_Msg.m_Data, a_Msg.m_Size));
        cereal::XMLInputArchive inputAr( buffer );
        Function function;
        inputAr(function);
        PRINT_VAR(function.m_Name);
    } );
}