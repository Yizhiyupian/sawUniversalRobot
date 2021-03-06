/*-*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-   */
/*ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab:*/

/*
(C) Copyright 2016-2017 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstConfig.h>
#include <cisstVector/vctRodriguezRotation3.h>
#include <cisstCommon/cmnLogger.h>
#include <cisstCommon/cmnKbHit.h>
#include <cisstCommon/cmnGetChar.h>
#include <cisstCommon/cmnConstants.h>
#include <cisstOSAbstraction/osaSleep.h>
#include <cisstMultiTask/mtsTaskContinuous.h>
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <sawUniversalRobot/mtsUniversalRobotScriptRT.h>


class UniversalRobotClient : public mtsTaskMain {

private:
    prmPositionJointGet jtpos;
    prmPositionCartesianGet cartpos;
    vctDoubleVec jtgoal, jtvel;
    prmPositionJointSet jtposSet;
    prmPositionCartesianSet cartposSet;
    prmVelocityJointSet jtvelSet;
    prmVelocityCartesianSet cartVelSet;

    mtsFunctionRead GetControllerTime;
    mtsFunctionRead GetControllerExecTime;
    mtsFunctionRead GetPositionJoint;
    mtsFunctionRead GetPositionCartesian;
    mtsFunctionRead GetConnected;
    mtsFunctionRead GetVersion;
    mtsFunctionRead GetAveragePeriod;
    mtsFunctionWrite PositionMoveJoint;
    mtsFunctionWrite PositionMoveCartesian;
    mtsFunctionWrite VelocityMoveJoint;
    mtsFunctionWrite VelocityMoveCartesian;
    mtsFunctionRead GetDebug;
    mtsFunctionVoid SetRobotFreeDriveMode;
    mtsFunctionVoid SetRobotRunningMode;
    mtsFunctionVoid StopMotion;
    mtsFunctionVoid DisableMotorPower;

    bool debugMode;

    void OnSocketError(void) {
        std::cout << std::endl << "Socket error communicating with robot" << std::endl;
    }
    void OnRobotNotReady(void) {
        std::cout << std::endl << "Robot not ready for motion command" << std::endl;
    }
    void OnReceiveTimeout(void) {
        std::cout << std::endl << "Timeout receiving data from robot" << std::endl;
    }
    void OnPacketInvalid(const vctULong2 &len) {
        std::cout << std::endl << "Invalid packet from robot, numBytes = " << len[0]
                  << ", packageLength = " << len[1] << std::endl;
    }

public:

    UniversalRobotClient() : mtsTaskMain("UniversalRobotClient"),
                             jtpos(6), jtgoal(6), jtvel(6), jtposSet(6), jtvelSet(6),
                             debugMode(false)
    {
        mtsInterfaceRequired *req = AddInterfaceRequired("Input", MTS_OPTIONAL);
        if (req) {
            req->AddFunction("GetControllerTime", GetControllerTime);
            req->AddFunction("GetControllerExecTime", GetControllerExecTime);
            req->AddFunction("GetPositionJoint", GetPositionJoint);
            req->AddFunction("GetPositionCartesian", GetPositionCartesian);
            req->AddFunction("GetConnected", GetConnected);
            req->AddFunction("GetAveragePeriod", GetAveragePeriod);
            req->AddFunction("JointPositionMove", PositionMoveJoint);
            req->AddFunction("CartesianPositionMove", PositionMoveCartesian);
            req->AddFunction("JointVelocityMove", VelocityMoveJoint);
            req->AddFunction("CartesianVelocityMove", VelocityMoveCartesian);
            req->AddFunction("GetDebug", GetDebug);
            req->AddFunction("GetVersion", GetVersion);
            req->AddFunction("SetRobotFreeDriveMode", SetRobotFreeDriveMode);
            req->AddFunction("SetRobotRunningMode", SetRobotRunningMode);
            req->AddFunction("StopMotion", StopMotion);
            req->AddFunction("DisableMotorPower", DisableMotorPower);
            req->AddEventHandlerVoid(&UniversalRobotClient::OnSocketError,
                                     this, "SocketError");
            req->AddEventHandlerVoid(&UniversalRobotClient::OnRobotNotReady,
                                     this, "RobotNotReady");
            req->AddEventHandlerVoid(&UniversalRobotClient::OnReceiveTimeout,
                                     this, "ReceiveTimeout");
            req->AddEventHandlerWrite(&UniversalRobotClient::OnPacketInvalid,
                                     this, "PacketInvalid");
        }
    }

    void Configure(const std::string&) {}

    void Startup()
    {
        std::cout << "m: position move joints" << std::endl
            << "M: position move cartesian" << std::endl
            << "v: velocity move joints" << std::endl
            << "V: velocity move cartesian" << std::endl
            << "d: toggle debug data display" << std::endl
            << "s: stop motion" << std::endl
            << "x: get version" << std::endl
            << "f: free drive mode" << std::endl
            << "r: running mode" << std::endl
            << "q: quit" << std::endl
            << "n: shut down robot" << std::endl
            << "Select Option: " << std::endl;
    }

    void Run() {

        bool connected = false;
        double period, cTime, cExecTime;
        vct3 velxyz, velrot;
        vct3 cartPos, cartVec;
        vctDoubleRot3 cartRot;
        vct6 debug;
        int version;
        const char *versionString[] = { "Unknown", "Pre-1.8", "1.8", "3.0/3.1", "3.2" };

        ProcessQueuedEvents();

        GetDebug(debug);
        GetControllerTime(cTime);
        GetControllerExecTime(cExecTime);
        GetPositionJoint(jtpos);
        GetPositionCartesian(cartpos);
        GetConnected(connected);
        GetAveragePeriod(period);

        if (connected) {
            if (cmnKbHit()) {
                char c = cmnGetChar();
                switch (c) {

                case 'm':   // position move joint
                    std::cout << std::endl << "Enter joint positions (deg): ";
                    std::cin >> jtgoal[0] >> jtgoal[1] >> jtgoal[2] >> jtgoal[3] >> jtgoal[4] >> jtgoal[5];
                    jtgoal.Multiply(cmnPI_180);
                    jtposSet.SetGoal(jtgoal);
                    PositionMoveJoint(jtposSet);
                    break;

                case 'M':   // position move Cartesian
                    std::cout << std::endl << "Enter Cartesian positions (mm): ";
                    std::cin >> cartPos[0] >> cartPos[1] >> cartPos[2];
                    cartPos.Divide(1000.0);  // convert from mm to m
                    cartposSet.SetGoal(cartPos);
                    std::cout << std::endl << "Enter Cartesian orientation (Rodriguez; 0,0,0 to skip): ";
                    std::cin >> cartVec[0] >> cartVec[1] >> cartVec[2];
                    if (cartVec.Any()) {
                        vctRodriguezRotation3<double> rot(cartVec);
                        cartRot.From(rot);
                    }
                    else
                        cartRot.Assign(cartpos.Position().Rotation());
                    cartposSet.SetGoal(cartRot);
                    PositionMoveCartesian(cartposSet);
                    break;

                case 'v':   // velocity move joint
                    std::cout << std::endl << "Enter joint velocities (deg/sec): ";
                    std::cin >> jtvel[0] >> jtvel[1] >> jtvel[2] >> jtvel[3] >> jtvel[4] >> jtvel[5];
                    jtvel.Multiply(cmnPI_180);
                    jtvelSet.SetGoal(jtvel);
                    VelocityMoveJoint(jtvelSet);
                    break;

                case 'V':  // velocity move Cartesian
                    std::cout << std::endl << "Enter Cartesian XYZ velocities (mm/sec): ";
                    std::cin >> velxyz[0] >> velxyz[1] >> velxyz[2];
                    velxyz.Divide(1000.0);  // Convert from mm to m
                    std::cout << std::endl << "Enter Cartesian angular velocities (deg/sec): ";
                    std::cin >> velrot[0] >> velrot[1] >> velrot[2];
                    velrot.Multiply(cmnPI_180);
                    cartVelSet.SetTranslationGoal(velxyz);
                    cartVelSet.SetRotationGoal(velrot);
                    VelocityMoveCartesian(cartVelSet);
                    break;

                case 'q':   // quit program
                    std::cout << "Exiting.. " << std::endl;
                    this->Kill();
                    break;

                case 'd':  // display debug data
                    debugMode = !debugMode;
                    break;

                case 's':   // stop motion
                    StopMotion();
                    break;

                case 'x':   // get version
                    GetVersion(version);
                    if ((version < 0) || (version >= sizeof(versionString)/sizeof(versionString[0])))
                        std::cout << std::endl << "Firmware version, invalid response = " << version << std::endl;
                    else
                        std:: cout << std::endl << "Firmware version: " << versionString[version] << std::endl;
                    break;

                case 'f':   // free drive mode
                    SetRobotFreeDriveMode();
                    break;

                case 'r':   // running mode
                    SetRobotRunningMode();
                    break;

                case 'n':   // shut down robot
                    DisableMotorPower();
                    break;
                }
            }

            vctDoubleVec jtposDeg(jtpos.Position());
            jtposDeg.Multiply(cmn180_PI);
            if (debugMode)
               printf("DEBUG: [%6.1lf,%6.1lf,%6.1lf,%6.1lf,%6.1lf,%6.1lf]                           \r",
                      debug[0], debug[1], debug[2], debug[3], debug[4], debug[5]);
#if 1
            else
               printf("JOINTS (deg): [%5.2lf,%5.2lf,%5.2lf,%5.2lf,%5.2lf,%5.2lf], PERIOD (s): %6.4lf\r",
                      jtposDeg[0], jtposDeg[1], jtposDeg[2], jtposDeg[3], jtposDeg[4], jtposDeg[5], period);
#else
            else
                printf("TIME (s): %6.1lf, JOINTS (deg): [%5.2lf,%5.2lf,%5.2lf,%5.2lf,%5.2lf,%5.2lf]\r",
                       cTime, jtposDeg[0], jtposDeg[1], jtposDeg[2], jtposDeg[3], jtposDeg[4], jtposDeg[5]);
#endif

        }
        osaSleep(0.01);  // to avoid taking too much CPU time
    }

    void Cleanup() {}

};


int main(int argc, char **argv)
{
    cmnLogger::SetMask(CMN_LOG_ALLOW_ALL);
    cmnLogger::SetMaskDefaultLog(CMN_LOG_ALLOW_ALL);
    cmnLogger::SetMaskFunction(CMN_LOG_ALLOW_ALL);
    cmnLogger::SetMaskClass("mtsUniversalRobotScriptRT", CMN_LOG_ALLOW_ALL);
    cmnLogger::AddChannel(std::cout, CMN_LOG_ALLOW_ERRORS_AND_WARNINGS);

    if (argc < 2) {
        std::cerr << "Syntax: URconsole <ip>" << std::endl;
        std::cerr << "        <ip>        IP address of Universal Robot" << std::endl;
        return 0;
    }
    mtsUniversalRobotScriptRT *urServer;
    urServer = new mtsUniversalRobotScriptRT("URserver");
    urServer->Configure(argv[1]);

    mtsComponentManager * componentManager = mtsComponentManager::GetInstance();
    componentManager->AddComponent(urServer);

    UniversalRobotClient client;
    componentManager->AddComponent(&client);

    if (!componentManager->Connect(client.GetName(), "Input", urServer->GetName(), "control")) {
        std::cout << "Failed to connect: "
            << client.GetName() << "::Input to "
            << urServer->GetName() << "::control" << std::endl;
        delete urServer;
        return -1;
    }

    componentManager->CreateAll();
    componentManager->StartAll();

    // Main thread passed to client task

    urServer->Kill();
    componentManager->WaitForStateAll(mtsComponentState::FINISHED, 2.0 * cmn_s);

    componentManager->Cleanup();

    // stop all logs
    cmnLogger::SetMask(CMN_LOG_ALLOW_NONE);
    delete urServer;

    return 0;
}
