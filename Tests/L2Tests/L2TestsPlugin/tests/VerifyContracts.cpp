#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <mutex>
#include <condition_variable>
#include <fstream>
#include "Network.h"
#include <time.h>

#define JSON_TIMEOUT   (1000)
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using namespace std;

/* Contract test verification test class declaration */
class CTVerifier_Test : public L2TestMocks {
protected:
    CTVerifier_Test();
    virtual ~CTVerifier_Test() override;

public:
    void createDeviceFiles();
    void enableTTS();
    void activateAllPlugins();
    void deactivateAllPlugins();
    bool parseContractTestResults(string& git_hash_str);
    // Add all supported plugins here
    vector<string> vtrPlugins = {
        "org.rdk.HdcpProfile",
        "org.rdk.Network", "org.rdk.System", "org.rdk.TextToSpeech"};
};

/**
 * @brief Constructor for CTVerifier_Test class
 */
CTVerifier_Test::CTVerifier_Test()
        : L2TestMocks()
{
}

/**
 * @brief Destructor for CTVerifier_Test class
 */
CTVerifier_Test::~CTVerifier_Test()
{
}

void CTVerifier_Test::activateAllPlugins()
{
    createDeviceFiles();
    /* Activate all plugins */
    for (vector<string>::iterator i = vtrPlugins.begin(); i != vtrPlugins.end(); i++)
    {
        cout << "Activating plugin: " << *i  << endl;
        uint32_t status = ActivateService((*i).c_str());
        EXPECT_EQ(Core::ERROR_NONE, status);
    }
    enableTTS();
}

void CTVerifier_Test::deactivateAllPlugins()
{
    /* Deactivate all plugins */
    for (vector<string>::iterator i = vtrPlugins.begin(); i != vtrPlugins.end(); i++)
    {
        cout << "Deactivating plugin: " << *i  << endl;
        uint32_t status = DeactivateService((*i).c_str());
        EXPECT_EQ(Core::ERROR_NONE, status);
    }
}

/* Contract test verification test class declaration */
class CTVerifier_CallMocks : public CTVerifier_Test {

protected:
    CTVerifier_CallMocks()
    {
        string videoPort(_T("HDMI0"));
        ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
            .WillByDefault(::testing::Return(videoPort));
        ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
            .WillByDefault(::testing::ReturnRef(device::VideoOutputPort::getInstance()));
        ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
        ON_CALL(*p_videoOutputPortMock, getHDCPStatus())
            .WillByDefault(::testing::Return(dsHDCP_STATUS_AUTHENTICATED));
        ON_CALL(*p_videoOutputPortMock, isContentProtected())
            .WillByDefault(::testing::Return(true));
        ON_CALL(*p_videoOutputPortMock, getHDCPReceiverProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));
        ON_CALL(*p_videoOutputPortMock, getHDCPCurrentProtocol())
            .WillByDefault(::testing::Return(dsHDCP_VERSION_2X));

        ON_CALL(*p_iarmBusImplMock, IARM_Bus_Call)
            .WillByDefault(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                if(string(ownerName) == string(_T(IARM_BUS_NM_SRV_MGR_NAME)))
                {
                    if (string(methodName) == string(_T(IARM_BUS_NETSRVMGR_API_getInterfaceList))) {
                        auto param = static_cast<IARM_BUS_NetSrvMgr_InterfaceList_t *>(arg);
                        param->size = 2;
                        memcpy(&param->interfaces[0].name, "eth0", sizeof("eth0"));
                        memcpy(&param->interfaces[0].mac, "AA:AA:AA:AA:AA:AA", sizeof("AA:AA:AA:AA:AA:AA"));
                        param->interfaces[0].flags = 69699;
                        memcpy(&param->interfaces[1].name, "wlan0", sizeof("wlan0"));
                        memcpy(&param->interfaces[1].mac, "AA:AA:AA:AA:AA:AA", sizeof("AA:AA:AA:AA:AA:AA"));
                        param->interfaces[1].flags = 69699;
                    }
                    else if (string(methodName) == string(_T(IARM_BUS_NETSRVMGR_API_isConnectedToInternet))) {
                        *((bool*) arg) = true;
                        EXPECT_EQ(*((bool*) arg), true);
                    }
                }
                return IARM_RESULT_SUCCESS;
            });
    }
    virtual ~CTVerifier_CallMocks() override
    {
    }
};

/**
 * @brief Create device properties and version files needed by various plugins
 */
void CTVerifier_Test::createDeviceFiles()
{
    std::ofstream deviceProperties("/etc/device.properties");
    deviceProperties << "BUILD_TYPE=dev\n";
    deviceProperties << "FORCE_MTLS=true\n";
    deviceProperties << "DEVICE_NAME=PLATCO\nDEVICE_TYPE=mediaclient\nMFG_NAME=Element\nWIFI_SUPPORT=true\n";
    deviceProperties << "MOCA_INTERFACE=true\nWIFI_INTERFACE=wlan0\nMOCA_INTERFACE=eth0\nETHERNET_INTERFACE=eth0\n";
    deviceProperties.close();

    std::ofstream version("/version.txt");
    version << "imagename:XUSHTC11MWR_VBN_2401_sprint_20240211231329sdy_NG\n";
    version << "VERSION=7.0.0.0\nBUILD_TIME=2024-02-11 23:13:29\n";
    version.close();
}

/**
 * @brief Enable TTS with dummy configuration
 */
void CTVerifier_Test::enableTTS()
{
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject result;
    JsonObject params_ttsen;
    JsonObject enableTTS;
    bool enable = true;
    enableTTS["enabletts"] = JsonValue((bool)enable);
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "enabletts", enableTTS, result);
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "isttsenabled", params_ttsen, result);

    JsonObject setTTSConfig;
    setTTSConfig["language"] = "en-US";
    setTTSConfig["voice"] = "carol";
    setTTSConfig["ttsendpoint"] = "https://sky-tts-run-api-gateway-3m02gblb.ew.gateway.dev/tts";
    setTTSConfig["ttsendpointsecured"] = "https://sky-tts-run-api-gateway-3m02gblb.ew.gateway.dev/tts";
    setTTSConfig["volume"] = "95";
    setTTSConfig["primvolduckpercent"] = "50";
    setTTSConfig["rate"] = "50";
    setTTSConfig["speechrate"] = "medium";
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "setttsconfiguration", setTTSConfig, result);
    status = InvokeServiceMethod("org.rdk.TextToSpeech.1", "getttsconfiguration", params_ttsen, result);
}

class CTVerifierMain : public CTVerifier_CallMocks {
};

/********************************************************
************Test case Details **************************
** 1. Run verify contracts using the pact_verifier_cli
*******************************************************/

TEST_F(CTVerifierMain, VerifyContractsOnAllPlugins)
{
    char const *pact_token = getenv("PACTFLOW_TOKEN");
    string pact_token_str = "";
    if (pact_token == NULL)
    {
        std::cout << "PACTFLOW_TOKEN is NULL\n";
    }
    else
    {
        std::string s(pact_token);
        pact_token_str = s;
    }

    if(pact_token_str.empty())
    {
        cout << "PACTFLOW_TOKEN is empty, don't run contract tests\n";
        return;
    }

    activateAllPlugins();

    //get short hash from long hash in GITHUB_SHA
    string git_hash_str = "";
    char const *l_hash = getenv("GITHUB_SHA");
    if (l_hash == NULL)
    {
        cout << "GITHUB_SHA is NULL\n";
    }
    else
    {
        cout << "GITHUB_SHA: " << l_hash << "\n";
        std::string s(l_hash);
        git_hash_str = s.substr(0, 7);
    }
    cout << "git_hash: " << git_hash_str << "\n";

    string test_pact_cmd = "~/bin/pact_verifier_cli --version";
    system(test_pact_cmd.c_str());

    string pact_verify_cmd = "~/bin/pact_verifier_cli ";
    pact_verify_cmd += "--loglevel=info --provider-name=rdk_service ";
    pact_verify_cmd += "--broker-url=https://skyai.pactflow.io --token=" + pact_token_str + " ";
    pact_verify_cmd += "--hostname=127.0.0.1 --port=9998 ";
    pact_verify_cmd += "--transport=websocket --publish ";
    pact_verify_cmd += "--provider-version=0.1.0-" + git_hash_str + " --provider-branch=test-github-rdkv ";
    pact_verify_cmd += "--filter-consumer ripple ";
    pact_verify_cmd += "--json contract-test-results.json ";
    //pact_verify_cmd += R"(--consumer-version-selectors="{\"tag\": \"0.1.0-4a18973\", \"latest\": true}" )";
    pact_verify_cmd += R"(--consumer-version-selectors="{\"mainBranch\": true}" )";

    cout << "pact_verify_cmd: " << pact_verify_cmd << "\n";
    int stat = system(pact_verify_cmd.c_str());
    cout << "pact_verify_cmd stat: " << stat << "\n";
    //EXPECT_GE(stat, 0);

    deactivateAllPlugins();


    //Parse the contract-test-results.json file and check if all the tests passed
    parseContractTestResults(git_hash_str);
}

/*
 * @brief Parse the contract-test-results.json file and write the results to contract-test-statistics.txt
 */
bool CTVerifier_Test::parseContractTestResults(string& git_hash_str)
{
    //Read json file to str::string
    std::ifstream t("contract-test-results.json");
    //If file doesn't exist, return
    if (!t.is_open())
    {
        std::cout << "contract-test-results.json file not found\n";
        return false;
    }
    std::string input((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    // std::cout << "input: " << input << std::endl;

    JsonObject parameters; 
    parameters.FromString(input);
    // std::string temp;
    // parameters.ToString(temp);
    // std::cout << "parameters: " << temp << std::endl;
    JsonArray err_arr = parameters["errors"].Array();
    uint32_t failed_apis = err_arr.Length();
    std::cout << "failed_apis: " << err_arr.Length() << std::endl;

    //Find number of occurences of "Test Name:" in a string input
    std::string search_str = "Test Name:";
    size_t pos = 0;
    int count = 0;
    while ((pos = input.find(search_str, pos)) != std::string::npos) {
        count++;
        pos += search_str.length();
    }
    std::cout << "Total Interactions: " << count << std::endl;

    //Get current date & time to a string
    time_t now = time(0);
    string dt = ctime(&now);
    cout << "The local date and time is: " << dt << endl;

    //Write test results in to a file
    std::ofstream test_results("contract-test-statistics.txt");
    test_results << "********************Contract Test Results********************" << "\n";
    test_results << "Date & Time: " << dt << "\n";
    test_results << "rdkservices version: " << git_hash_str << "\n\n";
    test_results << "Consumer: Ripple - Provider: rdk_service" << "\n";
    test_results << "Total Interactions: " << count << "\n";
    test_results << "Failed Interactions: " << failed_apis << "\n";
    test_results << "*************************************************************" << "\n";
    test_results.close();
    //return true if all tests passed
    return (failed_apis == 0);
}