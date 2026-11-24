#include "CubeBraidSDK/PLC_SDK/PLC_SDK.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <bitset>
#include <string>
#include <sstream>

// =================================================================
// 多吸盘阀组测试：确认操作、提交选择、等待后台写入结果
// =================================================================
static bool confirm(const char* message)
{
    std::cout << message << " [输入 YES 确认]: " << std::flush;
    std::string answer;
    return std::getline(std::cin, answer) && answer == "YES";
}

static bool selectSuctionCupAndWait(plc_sdk::PLCController& controller,
                                   int mask, int timeout_ms = 2000)
{
    if (mask < 1 || mask > 15 || timeout_ms <= 0) return false;
    if (!controller.setSuctionCupSelection(mask))
    {
        std::cerr << "吸盘选择请求未受理，请检查连接和互锁状态。" << std::endl;
        return false;
    }
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeout_ms);
    while (controller.isConnected() && std::chrono::steady_clock::now() < deadline)
    {
        const auto status = controller.getSuctionCupSelectionStatus();
        if (status.selection_mask != mask) return false;
        if (status.write_state == PLC_SUCTION_SELECTION_WRITTEN)
        {
            std::cout << "DB22.DBW48 写入成功：" << mask << " ("
                      << std::bitset<4>(mask) << ")" << std::endl;
            return true;
        }
        if (status.write_state == PLC_SUCTION_SELECTION_FAILED ||
            status.write_state == PLC_SUCTION_SELECTION_CANCELLED) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    // 超时不是取消，禁止继续开吸盘或直接重发；应检查PLC与SDK状态。
    std::cerr << "等待选择写入超时或断线，禁止继续开吸盘。" << std::endl;
    return false;
}

int main() 
{
    // 四组阀对应吸盘数量：3、3、3、4；编码为1、2、4、8。
    // 组合示例：阀1+阀2=3，阀2+阀3=6，阀4=8，全部=15。
    // 仅用于空载、隔离的联调工位；不要用于实际搬运流程。
    std::cout << "请输入吸盘组合编码 (1~15，默认3): ";
    std::string input;
    if (!std::getline(std::cin, input)) return 1;
    int mask = 3;
    if (!input.empty())
    {
        std::istringstream parser(input);
        char extra;
        if (!(parser >> mask) || (parser >> extra) || mask < 1 || mask > 15)
        {
            std::cerr << "无效编码，禁止发送。" << std::endl;
            return 1;
        }
    }
    std::cout << "选择编码：" << mask << " (" << std::bitset<4>(mask) << ")" << std::endl;
    if (!confirm("确认处于隔离空载工位、无工件吸持、PLC允许安全切换阀组")) return 0;

    // 实例化控制器
    plc_sdk::PLCController controller;

    // 1. 连接 PLC (IP, rack, slot)
    std::cout << "正在连接 PLC..." << std::endl;
    if (!controller.connect("192.168.30.49", 0, 1)) 
    {
        std::cerr << "连接 PLC 失败！" << std::endl;
        return -1;
    }
    std::cout << "PLC 连接成功！" << std::endl;

    // 原有初始化与取料参数示例保留，按需执行；初始化可能引起设备动作。
    if (confirm("是否在空载安全条件下测试原有PLC初始化及取料参数下发"))
    {
        // 触发 PLC 初始化脉冲
        std::cout << "正在触发 PLC 初始化..." << std::endl;
        if (controller.triggerPLCInit()) 
        {
            std::cout << "初始化指令已发送，等待 PLC 完成初始化..." << std::endl;
        }
        else
        {
            std::cerr << "初始化指令发送失败，停止测试。" << std::endl;
            return 1;
        }

        // 循环等待 PLC 初始化完成信号 (plc_init_state == 1)
        int timeout_ticks = 20;  // 500ms × 20 = 10秒
        while (timeout_ticks > 0) 
        {
            PLCStatus status = controller.getStatus();
            if (status.plc_init_state == 1) 
            {
                std::cout << "PLC 初始化成功！" << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            timeout_ticks--;
        }

        if (timeout_ticks <= 0) 
        {
            std::cerr << "PLC 初始化响应超时，停止测试！" << std::endl;
            return 1;
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(10000));

        // 发送取料参数数据 
        PickUpData pickup_data;
        pickup_data.fetch_mode = 4;     // 取料模式 (3/4 -> 理料模式3)
        pickup_data.mode_switch = 2;
        pickup_data.sku_l = 600;        // 长度 (mm)
        pickup_data.sku_w = 500;        // 宽度 (mm)
        pickup_data.sku_h = 400;        // 高度 (mm)
        pickup_data.sku_num = 1;        // 数量
        pickup_data.sku_weight = 5.5;   // 单个重量 (kg)

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (controller.sendPickUpData(pickup_data)) 
        {
            std::cout << "取料数据发送成功！" << std::endl;
        } 
        else 
        {
            std::cerr << "取料数据发送失败！" << std::endl;
            return 1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }

    // 新增：先选择阀组并等待DBW48写入成功，不直接打开吸盘。
    // 等效调用示例（独立使用，不要连续提交多个选择请求）：
    // controller.setSuctionCupSelection(0b0011);
    // controller.setSuctionCupSelection(plc_sdk::PLCController::VALVE_1 |
    //                                   plc_sdk::PLCController::VALVE_2);
    // controller.setSuctionCupValves(true, true, false, false);
    if (!selectSuctionCupAndWait(controller, mask))
    {
        std::cerr << "选择失败或结果不确定，停止本次测试。" << std::endl;
        controller.disconnect();
        return 1;
    }
    // WRITTEN仅表示通信写入成功，必须通过PLC/现场确认实际阀组生效。
    if (!confirm("已确认PLC实际应用选择、夹具空载且允许打开吸盘，是否测试开吸盘"))
    {
        controller.disconnect();
        return 0;  // 仅测试选择，不发送开关命令
    }
    if (!controller.requestOpenSuctionCup())
    {
        std::cerr << "开吸盘请求未受理，停止测试。" << std::endl;
        controller.disconnect();
        return 1;
    }
    std::cout << "开吸盘请求已受理（不是动作成功反馈）。" << std::endl;

    // 原有业务接口示例保留，仅在对应现场条件满足时执行。
    if (confirm("确认底托运动区域安全且PLC允许推出底托"))
        controller.setFixtureRollOut(true);
    if (confirm("确认倾角仪确有异常，是否发送异常信号"))
        controller.setInclinometerErrorSignal(true);
    if (confirm("确认真实吸取OK反馈及业务联锁均满足，是否发送吸取OK"))
        controller.setSuctionCupPickingOK(true);
    if (confirm("确认机器人实际取料完成且允许发送完成信号"))
        controller.setRetrievingCompletionSide(true);
    if (confirm("确认摆台运动区域安全且允许切换角度模式"))
        controller.controlTableAngle(1);
    if (confirm("确认允许写入原有演示业务数据"))
    {
        controller.updateBoxState(6, 9, 4);        // 面6、层9、箱4
        controller.updateInclinometerAngle(-3.5f); // 倾角-3.5°
        controller.updateTotalBoxState(1000, 500); // 总箱数1000、剩余500
    }

    // 4. 主线程循环获取后台线程读取到的实时 PLC 状态
    for (int i = 0; i < 2; ++i) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        PLCStatus status = controller.getStatus();
        std::cout << "--- 实时状态监控 [" << i + 1 << "] ---" << std::endl;
        std::cout << "吸盘状态: " << status.suction_cup_state << std::endl;
        std::cout << "夹具/底托状态: " << status.fixture_state << std::endl;
        std::cout << "机器人取料状态: " << status.robot_pick_state << std::endl;
        std::cout << "机器人离开状态: " << status.robot_leave_state << std::endl;

        std::cout << "摆台控制状态: " << status.table_control_state << std::endl;
        std::cout << "液压杆上升状态: " << status.hydraulic_rod_rise_state << std::endl;
        std::cout << "液压杆下降状态: " << status.hydraulic_rod_lower_state << std::endl;
        std::cout << "PLC初始化状态: " << status.plc_init_state << std::endl;
        std::cout << "摆台初始化完成状态: " << status.table_init_control_ok_state << std::endl;
        
        std::cout << "PLC请求数据状态: " << status.plc_data_request_state << std::endl;
    }

    // 关闭吸盘需要确认无悬空工件；不能因异常自动释放。
    if (confirm("确认无悬空工件且允许关闭吸盘"))
    {
        controller.setCloseSuctionCup(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "关闭请求已提交，请核实PLC及实际真空释放状态。" << std::endl;
    }
    else
        std::cerr << "未发送关闭命令，请在现场按安全流程处理夹具。" << std::endl;
    if (confirm("确认底托运动区域安全且允许缩回底托"))
    {
        controller.setFixtureInitialization(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }              

    // 5. 断开连接并回收后台线程（断线不代表吸盘已关闭）
    std::cout << "断开 PLC 连接..." << std::endl;
    controller.disconnect();

    return 0;
}