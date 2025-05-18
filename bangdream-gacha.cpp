#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <numeric>
#include "color.h"

#define Simulations 1000000 //定义模拟次数为一百万次

//定义一个随机数生成器类，用于模拟抽卡
class GachaRandom {
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    std::uniform_int_distribution<> dis_5star;

public:
    GachaRandom(int total_5star) 
        : gen(rd())
        , dis(0.0, 1.0)
        , dis_5star(1, total_5star) 
    {}

    // 获取0-1之间的随机数
    double get_random() {
        return dis(gen);
    }

    // 获取5星卡的随机编号
    int get_5star_random() {
        return dis_5star(gen);
    }
};

typedef struct arg_processing_return {
    bool reverse_flag = 0;
    bool processed_normally = 0;
    int simulations = Simulations;
} apr;

int simulate_one_round(int total_5star, int want_5star, int total_4star, int want_4star, int normal, GachaRandom& random) {
    std::set<int> cards_5star;  // 已拥有的5星卡片集合
    std::set<int> cards_4star;  // 已拥有的4星卡片集合
    int draws = 0;  // 已经抽卡次数
    int choose_times_have = 0; // 拥有的自选次数

    while (true) {
        draws++;
        // 修改50保底逻辑
        if (draws % 50 == 0 && normal == 1) {
            if (want_5star > 0) {  // 只有当我们想要5星卡时才考虑
                int roll = random.get_5star_random();  // 随机抽取一张5星卡
                if (roll <= want_5star) {  // 如果抽到的是想要的卡
                    cards_5star.insert(roll);
                }
            }
            goto next_draw;  // 50保底必定是5星，跳过4星判定
        }
        else {
            // 正常抽卡
            double rand = random.get_random();
            // 先判断5星
            if (want_5star > 0) {  // 只有当我们想要5星卡时才判定
                for (int i = 1; i <= want_5star; i++) {
                    if (rand < 0.005 * total_5star * (i / (double)total_5star)) {
                        cards_5star.insert(i);
                        goto next_draw;
                    }
                }
            }

            // 如果想要的5星数量为0，则必定想要4星
            //看似多此一举的一个else分支，可以为单线程100万次模拟减短大约一秒的计算时间（自豪）
            else {
                rand = random.get_random();
                for (int i = 1; i <= want_4star; i++) {
                    if (rand < 0.0075 * total_4star * (i / (double)total_4star)) {
                        cards_4star.insert(i);
                        break;
                    }
                }
                goto next_draw;  // 跳过重复的4星判定
            }

            // 再判断4星
            if (total_4star > 0 && want_4star > 0) {
                rand = random.get_random();
                for (int i = 1; i <= want_4star; i++) {
                    if (rand < 0.0075 * total_4star * (i / (double)total_4star)) {
                        cards_4star.insert(i);
                        break;
                    }
                }
            }
        }
        next_draw:

        // 自选逻辑
        // if (draws % 300 == 200) {
        //     if (cards_5star.size() < want_5star) {
        //         for (int i = 1; i <= want_5star; i++) {
        //             if (cards_5star.find(i) == cards_5star.end()) {
        //                 cards_5star.insert(i);
        //                 goto skip_4star_choice;
        //             }
        //         }
        //     }
        //     if (cards_4star.size() < want_4star) {
        //         for (int i = 1; i <= want_4star; i++) {
        //             if (cards_4star.find(i) == cards_4star.end()) {
        //                 cards_4star.insert(i);
        //                 break;
        //             }
        //         }
        //     }
        //     skip_4star_choice:;
        // }
        //新的自选逻辑，先把抽卡抽完，再进行自选，避免重复抽到
        choose_times_have = ( draws + 100 ) / 300;
        if(cards_5star.size() + cards_4star.size() + choose_times_have >= want_5star + want_4star) {
            break; 
        }
    }
    
    return draws;
}

// 修改统计函数签名，添加线程数参数
int calculate_statistics(int total_5star, int want_5star, int total_4star, int want_4star, int normal, 
                        int simulations, unsigned int thread_count, bool reverseFlag) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "----------------输入参数----------------" << std::endl;
    std::cout << "当期5星数量: " << total_5star << std::endl;
    std::cout << "想要的当期5星数量: " << want_5star << std::endl;
    std::cout << "想要的当期4星数量: " << want_4star << std::endl;
    std::cout << "50小保底: " << (normal == 1 ? "是" : "否") << std::endl;
    std::cout << "模拟次数: " << simulations << std::endl;
    std::cout << "使用线程数: " << thread_count << std::endl;
    std::cout << "计算中..." << std::endl;

    std::vector<int> draw_counts;
    draw_counts.reserve(simulations);
    std::mutex mtx;
    long long total_draws = 0;
    
    // 使用用户指定的线程数
    std::vector<std::thread> threads;
    int sims_per_thread = simulations / thread_count;
        
    // 创建多个线程执行模拟
    for (unsigned int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, i]() {
            GachaRandom random(total_5star);  // 每个线程使用独立的随机数生成器
            std::vector<int> local_draws;     // 线程本地存储
            local_draws.reserve(sims_per_thread);
            
            int start = i * sims_per_thread;
            int end = (i == thread_count - 1) ? simulations : (i + 1) * sims_per_thread;
            
            for (int j = start; j < end; ++j) {
                int draws = simulate_one_round(total_5star, want_5star, total_4star, want_4star, normal, random);
                local_draws.push_back(draws);
            }
            
            // 合并结果
            std::lock_guard<std::mutex> lock(mtx);
            draw_counts.insert(draw_counts.end(), local_draws.begin(), local_draws.end());
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 计算总抽数
    total_draws = std::accumulate(draw_counts.begin(), draw_counts.end(), 0LL);
    
    double expected_draws = static_cast<double>(total_draws) / simulations;
    std::sort(draw_counts.begin(), draw_counts.end());
    double percentile_50 = draw_counts[simulations / 2];
    double percentile_90 = draw_counts[static_cast<int>(simulations * 0.9)];
    double neineigeneinei = draw_counts.back();
    
    std::cout << "----------------模拟结果----------------" << std::endl;
    std::cout << "期望抽卡次数: " << ANSI_Cyan << expected_draws << std::endl;
    std::cout << "中位数抽卡次数: " << percentile_50 << ANSI_COLOR_RESET << "，即" << percentile_50 /40  << "w星石" << std::endl;
    std::cout << "90%玩家在以下抽数内集齐: " << ANSI_Cyan << percentile_90 << ANSI_COLOR_RESET << "，即" << percentile_90 /40 << "w星石" << std::endl;
    std::cout << "非酋至多抽卡次数: " << ANSI_Cyan << neineigeneinei << ANSI_COLOR_RESET << "，即" << neineigeneinei /40 << "w星石" << std::endl;
    // 结束计时并计算耗时
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double seconds = duration.count() / 1000.0;
    
    std::cout << "模拟耗时: " << seconds << " 秒" << std::endl;

    if(reverseFlag == 1) {
        double input, sigma, z, cdfValue;
        std::cout << "请输入所使用的抽数：";
        std::cin >> input;
        if(input < 10) {
            std::cerr << "输入值过小！" << std::endl;
            return -1;
        }
        sigma = (percentile_90 - expected_draws)/1.28155;
        z = (input - expected_draws)/sigma;
        cdfValue = 0.5 * (1 + std::erf(z / std::sqrt(2)));
        std::cout << "输入值 " << input << " 对应累积概率约为 " << ANSI_Cyan << cdfValue * 100.0 << "% 。" << ANSI_COLOR_RESET << std::endl;
    }
    return 0;
}

apr arg_processing(int argc, const char* argv[]) {
    apr Result;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--reverse" || arg == "-r") {
            Result.reverse_flag = 1;
            std::cout << ANSI_Red <<" 当前处于反推抽数排名模式，结果仅供参考" << ANSI_COLOR_RESET <<std::endl;
            } else if (arg == "--version" || arg == "-v") {
                std::cout << "Version 1.8,Build 39 \n"
                    << "Copyright (c) 2025, 山泥若叶睦，Modified by UDMH \n"
                    << "Original page at: https://gitee.com/handsome-druid/bangdream-gacha\n"
                    << "My GitHub page at: https://github.com/YukkimuraHinata/bangdream-gacha" << std::endl;
            } else if (arg == "--number" || arg == "-n") {
                i++;
                int tmpSimulations = std::atoi(argv[i]);
                if(tmpSimulations > Simulations){
                    Result.simulations = tmpSimulations;
                    }
                    else {
                        std::cout << "将使用默认值:" << Simulations << std::endl;
                    }
            }
            else if (arg == "--help" || arg == "-h") {
                std::cout << "Options: \n"
                << "  --reverse    -r    反推抽数排名\n"
                << "  --number     -n    指定模拟次数，不应少于100万次\n"
                << "  --version    -v    显示版本信息\n"
                << "  --help       -h    显示帮助" <<std::endl;
            } else {
                std::cerr << "未知参数: " << ANSI_Red << arg << ANSI_COLOR_RESET << std::endl;
                Result.processed_normally = 1;
            }
        }
    return Result;
}

int main(int argc, char* argv[]) {
    apr res = arg_processing(argc, const_cast<const char**>(argv));
    if(res.processed_normally == 1) {
        return res.processed_normally;
    }

    bool reverseFlag = res.reverse_flag;
    int isNormal = 1, simulations = res.simulations;
    int total_5star = 0, want_5star = 0, total_4star = 0, want_4star = 0;
    std::cout << ANSI_Blue_BG << "BanG Dream! Gacha,a gacha simulator of Garupa" << ANSI_COLOR_RESET << std::endl;
    std::cout << "请输入当期5星卡的总数量: ";
    std::cin >> total_5star;
    if (total_5star < 0) {
        std::cout << "卡片数量必须大于等于0！" << std::endl;
        return 1;
    }

    if (total_5star > 0) {
        std::cout << "请输入想要抽取的当期5星卡数量: ";
        std::cin >> want_5star;
        if (want_5star < 0 || want_5star > total_5star) {
            std::cout << "想要的卡片数量必须在0到" << total_5star << "之间！" << std::endl;
            return 1;
        }
    }

        std::cout << "请输入想要抽取的当期4星卡数量: ";
        std::cin >> want_4star;
        if (want_4star < 0) {
            std::cout << "不能小于0！" << std::endl;
            return 1;
        }
    total_4star = want_4star;
    std::cout << "是否为常驻池（是否有50小保底）（1: 是，0: 否）: ";
    std::cin >> isNormal;
    if (isNormal != 0 && isNormal != 1) {
        std::cout << "输入错误！（1: 是，0: 否）" << std::endl;
        return 1;
    }

    // 在main函数中修改线程数检测部分
    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) {
        thread_count = 4;
    }
    unsigned int user_threads;
    user_threads = std::max(1u,thread_count / 2);
/*
    std::cout << "输入使用的线程数（-1查看更多说明，0使用保守建议值：）";
    unsigned int user_threads;
    std::cin >> user_threads;
    if (user_threads == -1) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "检测到系统支持的并发线程数：" << thread_count << std::endl;
        std::cout << "注意：如果你使用的是Intel大小核处理器（12代或更新）：" << std::endl;
        std::cout << "1. 建议仅使用P核心数量的线程" << std::endl;
        std::cout << "2. 可以通过任务管理器->性能->CPU->处理器详细信息查看P核心数量" << std::endl;
        std::cout << "请输入想要使用的线程数（1-" << thread_count << "）\n";
        std::cout << "建议值：" << std::max(1u, thread_count / 4) << "-" << std::max(1u, thread_count / 2) 
                 << " （根据CPU架构选择合适的值）\n";
        std::cout << "输入0则使用保守建议值（"<< std::max(1u, thread_count / 4 )<< "）：";
        std::cin >> user_threads;
    }

    if (user_threads == 0) {
        // 使用更保守的默认值，避免在大小核架构上使用过多线程
        user_threads = std::max(1u, thread_count / 4);
        std::cout << "使用保守建议值：" << user_threads << " 线程\n";
    } else if (user_threads > thread_count) {
        std::cout << "警告：指定的线程数超过系统支持的最大值，将使用" << thread_count << "线程\n";
        user_threads = thread_count;
    } else if (user_threads < 1) {
        std::cout << "线程数必须大于0，将使用1个线程\n";
        user_threads = 1;
    }
*/
    calculate_statistics(total_5star, want_5star, total_4star, want_4star, isNormal, simulations, user_threads, reverseFlag);
    /*
    std::cout << "\n按回车键退出程序...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
    */
    return 0;
}
