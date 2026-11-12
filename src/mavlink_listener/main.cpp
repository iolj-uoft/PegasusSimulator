#include <mavsdk/mavsdk.hpp> //core: MAVSDK, sys, connection
#include <mavsdk/plugins/telemetry/telemetry.hpp> // gives you attitude directly, as Euler angles or a quaternion.
#include <mavsdk/plugins/action/action.hpp>
// #include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.hpp> // raw MAVLINK msgs. e.g., HEARTBEAT
#include <mavsdk/plugins/info/info.hpp>

#include <iostream>
#include <chrono>
#include <thread>

using namespace mavsdk;
using std::chrono::seconds;
using std::this_thread::sleep_for;

void usage(const std::string& bin_name) {
    std::cerr << "Usage : " << bin_name << " <connection_url>\n"
              << "Connection URL format should be :\n"
              << " For TCP server: tcpin://<our_ip>:<port>\n"
              << " For TCP client: tcpout://<remote_ip>:<port>\n"
              << " For UDP server: udpin://<our_ip>:<port>\n"
              << " For UDP client: udpout://<remote_ip>:<port>\n"
              << " For Serial : serial://</path/to/serial/dev>:<baudrate>]\n"
              << "For example, to connect to the simulator use URL: udpin://0.0.0.0:14540\n";
}

int main(int argc, char** argv) // argc: argument count
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    std::cout << "# of args: " << argc << ", 1st arg: " << argv[1] << std::endl;

    Mavsdk mavsdk{Mavsdk::Configuration{ComponentType::GroundStation}};
    ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << "\n";
        return 1;
    }

    std::cout << "Waiting to discover drone..." << std::endl;
    
    auto system = mavsdk.first_autopilot(3.0); // type: std::optional<std::shared_ptr<System>>
    if (!system) {
        std::cerr << "Timed out waiting for system\n";
        return 1;
    }

    auto telemetry = Telemetry{system.value()}; // passes a shared_ptr to the system (drone) and construct the plugin object
    auto action = Action{system.value()};

    system.value()->subscribe_is_connected([](bool is_connected) {
        if (is_connected) {
            std::cout << "[HEARTBEAT] Drone is connected!" << std::endl;
        } else {
            std::cout << "[HEARTBEAT] Connection lost (Heartbeat timeout)..." << std::endl; 
        }
    });

    telemetry.subscribe_armed([](bool is_armed) {
        std::printf("\n[ARM]: %s\n", is_armed ? "true" : "false");
    });

    // setup alttitude subscriber
    const auto set_alt_rate_result = telemetry.set_rate_position(1.0);
    if (set_alt_rate_result != Telemetry::Result::Success) {
        std::cerr << "Setting altitute rate failed: " << set_alt_rate_result << '\n';
        return 1;
    }
    
    telemetry.subscribe_position([](Telemetry::Position position) {
        std::cout << "[ALTITUDE] " << position.relative_altitude_m << "m\n";
    });

    // setup attitude subscriber
    const auto set_att_rate_result = telemetry.set_rate_attitude_euler(1.0);
    if (set_att_rate_result != Telemetry::Result::Success) {
        std::cerr << "Setting attitute rate failed: " << set_att_rate_result << '\n';
        return 1;
    }

    telemetry.subscribe_attitude_euler([](Telemetry::EulerAngle attitude) {
        std::printf("[ATTITUDE] Roll: %6.2f | Pitch: %6.2f | Yaw: %6.2f\n", 
            attitude.roll_deg,
            attitude.pitch_deg,
            attitude.yaw_deg);
    });

    // check until vehicle is ready to arm
    while (!telemetry.health_all_ok() || !telemetry.health().is_armable) {
        std::cout << "Vehicle is getting ready to arm\n";
        sleep_for(seconds(1));
    }

    // A prev flight might leave vehicle in land mode so PX4 won't arm. Switch to holding mode first.
    std::cout << "Switching to Hold...\n";
    const Action::Result initial_hold_result = action.hold();
    if (initial_hold_result != Action::Result::Success) {
        std::cerr << "Switching to Hold failed: " << initial_hold_result << "\n";
        return 1;
    }

    // Arm vehicle
    std::cout << "Arming...\n";
    const Action::Result arm_result = action.arm();

    if (arm_result != Action::Result::Success) {
        std::cerr << "Arming failed: " << arm_result << '\n';
        return 1;
    }

    // Take off
    std::cout << "Taking off...\n";
    const Action::Result takeoff_result = action.takeoff();

    if (takeoff_result != Action::Result::Success) {
        std::cerr << "Takeoff failed: " << takeoff_result << '\n';
        return 1;
    }

    // Hover for 10s
    sleep_for(seconds(10));

    // Landing
    std::cout << "Landing...\n";
    const Action::Result landing_result = action.land();

    if (landing_result != Action::Result::Success) {
        std::cerr << "Landing failed: " << landing_result << '\n';
        return 1;
    }

    // Check if vehicle is still in air
    while (telemetry.in_air()) {
        std::cout << "Vehicle is landing...\n";
        sleep_for(seconds(1));
    }
    std::cout << "Landed!\n";

    // auto-disarming but still watch the tele for a few more sec
    constexpr int DISARM_TIMEOUT_S = 10;
    for (size_t i = 0; i < DISARM_TIMEOUT_S && telemetry.armed(); ++i) {
        std::cout << "Waiting for auto-disarm...\n";
        sleep_for(seconds(1));
    }

    if (telemetry.armed()) {
        std::cout << "Not disarmed after " << DISARM_TIMEOUT_S << "s, disarming manually...\n";
        const Action::Result disarm_result = action.disarm();
        if (disarm_result != Action::Result::Success) {
            std::cerr << "Disarm failed: " << disarm_result << "\n";
            return 1;
        }
    }
    std::cout << "Finished...\n";


    return 0;
}

