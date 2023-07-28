#pragma once

#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <random>
#include <memory>
#include <chrono>
#include <thread>
#include <omp.h>

#include "graph.hpp"
#include "message_queue.hpp"
#include "start_flag.hpp"
#include "random_walk_config.hpp"
#include "random_walker_manager.hpp"
#include "random_walker.hpp"
#include "measurement.hpp"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class ARWS {

public :

    // コンストラクタ
    ARWS(const std::string& dir_path);

    // thread を開始させる関数
    void start();

    // RWer 生成 & 処理をする関数
    void generateRWer();

    // RW を実行する関数
    void executeRandomWalk(std::unique_ptr<RandomWalker>&& RWer_ptr);

    // メッセージ処理用の関数 (ポート番号毎)
    void procMessage(const uint16_t& port_num);

    // send_queue から RWer を取ってきて他サーバへ送信する関数 (送信先毎)
    void sendMessage(const uint32_t& dst_ip);

    // RWer を送信する関数
    void sendMessageFunc(RandomWalker& RWer, const uint32_t& dst_ip, int sockfd, const uint16_t& port_num);

    // 他サーバからメッセージを受信し, message_queue に push する関数 (ポート番号毎)
    void receiveMessage(int sockfd, const uint16_t& port_num);

    // IPv4 サーバソケットを生成 (UDP)
    int createUdpServerSocket(const uint16_t& port_num);

    // IPv4 サーバソケットを生成 (TCP)
    int createTcpServerSocket(const uint16_t& port_num);

    // 実験結果を start_manager に送信する関数
    void sendToStartManager();

    // 実験観察者
    void observer();

private :

    std::string hostname_; // 自サーバのホスト名
    uint32_t hostip_; // 自サーバの IP アドレス
    uint32_t startmanagerip_; // StartManager の IP アドレス
    Graph graph_; // グラフデータ
    std::unordered_map<uint16_t, MessageQueue<RandomWalker>> receive_queue_; // ポート番号毎の receive キュー
    std::unordered_map<uint32_t, MessageQueue<RandomWalker>> send_queue_; // 送信先毎の send キュー
    StartFlag start_flag_; // 実験開始の合図に関する情報
    RandomWalkConfig RW_config_; // Random Walk 実行関連の設定
    RandomWalkerManager RW_manager_; // RWer に関する情報
    Measurement measurement_; // 時間計測用

    std::vector<std::thread> observer_thread_; 
    bool observer_flag_ = true;

    // パラメタ (スレッド数)
    const uint32_t SEND_RECV_PORT = 16;
    const uint32_t SEND_PER_PORT = 1;
    const uint32_t RECV_PER_PORT = 1;
    const uint32_t PROC_MESSAGE_PER_PORT = 1;
    const uint32_t GENERATE_RWER = 1;

    // パラメタ (メッセージ長)
    const uint32_t MESSAGE_MAX_LENGTH = 1450;

    // パラメタ (メッセージ終了判定)
    const uint32_t MESSAGE_END = 1100200300;

    // 送信先IP
    std::vector<std::string> worker_ip_all_ = {"10.58.60.3", "10.58.60.5", "10.58.60.6", "10.58.60.7", "10.58.60.8"};
    std::vector<uint32_t> worker_ip_;

    // 乱数関連
    std::mt19937 mt{std::random_device{}()}; // メルセンヌ・ツイスタを用いた乱数生成
    std::uniform_real_distribution<double>  rand_double{0, 1.0}; // 0~1のランダムな値

};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

inline ARWS::ARWS(const std::string& dir_path) {
    // 自サーバ のホスト名
    char hostname_c[128]; // ホスト名
    gethostname(hostname_c, sizeof(hostname_c)); // ホスト名を取得
    hostname_ = hostname_c; // char* から string へ

    // 自サーバ の IP アドレス (NIC 依存)
    const char* ipname;
    if (hostname_[0] == 'a') ipname = "em1"; // abilene
    else if (hostname_[0] == 'e') ipname = "enp2s0"; // espresso
    else ipname = "vlan151"; // giji-4

    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ifr.ifr_addr.sa_family = AF_INET; // IPv4 の IP アドレスを取得したい
    strncpy(ifr.ifr_name, ipname, IFNAMSIZ-1); // ipname の IP アドレスを取得したい
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    hostip_ = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

    // グラフファイル読み込み
    graph_.init(dir_path, hostname_, hostip_);

    // 送信先IP
    for (std::string ip : worker_ip_all_) {
        uint32_t ip_int = inet_addr(ip.c_str());
        if (ip_int != hostip_) worker_ip_.push_back(ip_int);
    }

    // キューを初期化
    for (int i = 0; i < SEND_RECV_PORT; i++) {
        receive_queue_[10000+i].getSize();
    }
    // for (int i = 0; i < SEND_RECV_PORT; i++) {
    //     send_queue_[10000+i].getSize();
    // }
    for (uint32_t ip : worker_ip_) {
        send_queue_[ip].getSize();
    }


    // 全てのスレッドを開始させる
    start();
}

inline void ARWS::start() {

    std::thread thread_generateRWer(&ARWS::generateRWer, this);

    std::vector<std::thread> threads_sendMessage;
    // for (int i = 0; i < SEND_RECV_PORT; i++) {
    //     for (int j = 0; j < SEND_PER_PORT; j++) {
    //         threads_sendMessage.emplace_back(std::thread(&ARWS::sendMessage, this, 10000+i));
    //     }
    // }
    for (uint32_t ip : worker_ip_) {
        threads_sendMessage.emplace_back(std::thread(&ARWS::sendMessage, this, ip));
    }

    std::vector<std::thread> threads_receiveMessage;
    for (int i = 0; i < SEND_RECV_PORT; i++) {
        int sockfd = createUdpServerSocket(10000+i);
        for (int j = 0; j < RECV_PER_PORT; j++) {
            threads_receiveMessage.emplace_back(std::thread(&ARWS::receiveMessage, this, sockfd, 10000+i));
        }
    }

    std::vector<std::thread> threads_procMessage;
    for (int i = 0; i < SEND_RECV_PORT; i++) {
        for (int j = 0; j < PROC_MESSAGE_PER_PORT; j++) {
            threads_procMessage.emplace_back(std::thread(&ARWS::procMessage, this, 10000+i));
        }
    }

    // プログラムを終了させないようにする
    thread_generateRWer.join();

}

inline void ARWS::generateRWer() {
    std::cout << "generateRWer" << std::endl;

    // ソケットの生成
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { // エラー処理
        perror("socket");
        exit(1); // 異常終了
    }  

    while (1) {
        // 開始通知を受けるまでロック
        start_flag_.lockWhileFalse();

        std::cout << "start RW!" << std::endl;

        // observer 起動
        observer_flag_ = true;
        observer_thread_.push_back(std::thread(&ARWS::observer, this));

        uint32_t number_of_RW_execution = RW_config_.getNumberOfRWExecution();
        uint32_t number_of_my_vertices = graph_.getMyVerticesNum();
        std::vector<uint32_t> my_vertices = graph_.getMyVertices();

        RW_manager_.init(number_of_my_vertices * number_of_RW_execution);

        uint32_t num_threads = GENERATE_RWER;
        omp_set_num_threads(num_threads);

        measurement_.setStart();

        // debug 
        std::cout << number_of_my_vertices << std::endl;
        std::cout << number_of_RW_execution << std::endl;

        // uint32_t RWer_id = 0;
        int j;
        #pragma omp parallel for private(j) 
        for (int i = 0; i < number_of_my_vertices; i++) {

            uint32_t node_id = my_vertices[i];

            for (int j = 0; j < number_of_RW_execution; j++) {

                uint32_t RWer_id = i * number_of_RW_execution + j;

                // 生成スピード調整
                // RW_manager_.lockWhileOver();

                // RWer を生成
                std::unique_ptr<RandomWalker> RWer_ptr = std::make_unique<RandomWalker>(RandomWalker(node_id, RWer_id, hostip_));

                // 生成時刻を記録
                RW_manager_.setStartTime(RWer_id);

                // RW を実行 
                executeRandomWalk(std::move(RWer_ptr));

                // RWer_id++;

                // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        measurement_.setEnd();

        std::cout << measurement_.getExecutionTime() << std::endl;
    }
}

inline void ARWS::executeRandomWalk(std::unique_ptr<RandomWalker>&& RWer_ptr) {

    while (1) {
        // 現在ノードの隣接ノード集合を入手
        std::vector<uint32_t> adjacency_vertices = graph_.getAdjacencyVertices(RWer_ptr->getCurrentNode());

        // 次数
        uint32_t degree = adjacency_vertices.size();

        // RW を一歩進める
        if (rand_double(mt) < RW_config_.getAlpha() || degree == 0) { // 確率 α もしくは次数 0 なら終了

            RWer_ptr->endRWer();
            uint32_t RWer_hostip = RWer_ptr->getHostip();

            if (RWer_hostip == hostip_) { // もし終了した RWer の起点サーバが今いるサーバである場合, 終了した RWer をこの場で処理
      
                RW_manager_.setEndTime(RWer_ptr->getRWerId());
      
            } else { // そうでないなら send_queue に push

                // std::uniform_int_distribution<int> rand_port(10000, 10000+SEND_RECV_PORT-1);
                send_queue_[RWer_hostip].push(std::move(RWer_ptr));

            }

            break;

        } else { // 確率 1-α でランダムな隣接ノードへ遷移

            // ランダムな隣接ノードへ遷移
            std::uniform_int_distribution<int> rand_int(0, degree-1);
            uint32_t next_node = adjacency_vertices[rand_int(mt)];
            RWer_ptr->updateRWer(next_node);
            uint32_t next_node_ip = graph_.getIP(next_node);

            // 遷移先ノードの持ち主が自サーバか他サーバかで分類
            if (next_node_ip == hostip_) { // 自サーバへの遷移

                continue;

            } else { // 他サーバへの遷移 (メッセージを生成し, send_queue に push)

                // std::uniform_int_distribution<int> rand_port(10000, 10000+SEND_RECV_PORT-1);
                send_queue_[next_node_ip].push(std::move(RWer_ptr));

                break;

            }

        }
    } 
}

inline void ARWS::procMessage(const uint16_t& port_num) {
    std::cout << "procMessage: " << port_num << std::endl;

    while (1) {
        // メッセージキューからメッセージを取得
        std::unique_ptr<RandomWalker> RWer_ptr = receive_queue_[port_num].pop();

        if (RWer_ptr->getEndFlag() == 1) { // 終了した RWer の処理

            RW_manager_.setEndTime(RWer_ptr->getRWerId());

        } else { // まだ生存している RWer の処理

            // RW を実行
            executeRandomWalk(std::move(RWer_ptr));

        }

    }  
}

inline void ARWS::sendMessage(const uint32_t& dst_ip) {
    std::cout << "send_message: " << dst_ip << std::endl;

    // ソケットの生成
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { // エラー処理
        perror("socket");
        exit(1); // 異常終了
    }  

    // debug 
    // std::cout << 100 << std::endl;

    while (1) {
        // メッセージ宣言
        char message[MESSAGE_MAX_LENGTH + sizeof(MESSAGE_END)];
        // std::unique_ptr<char[]> message(new char[MESSAGE_MAX_LENGTH + MESSAGE_END]);

        // debug 
        // std::cout << 100 << std::endl;

        // キューからRWerを取ってきてメッセージに詰める
        uint32_t now_length = send_queue_[dst_ip].pop(message, MESSAGE_MAX_LENGTH);

        // メッセージ終端の目印を書き込む
        memcpy(message + now_length, &MESSAGE_END, sizeof(MESSAGE_END));
        now_length += sizeof(MESSAGE_END);

        // ポート番号をランダムに生成
        std::uniform_int_distribution<int> rand_port(10000, 10000+SEND_RECV_PORT-1);

        // アドレスの生成
        struct sockaddr_in addr; // 接続先の情報用の構造体(ipv4)
        memset(&addr, 0, sizeof(struct sockaddr_in)); // memsetで初期化
        addr.sin_family = AF_INET; // アドレスファミリ(ipv4)
        addr.sin_port = htons(rand_port(mt)); // ポート番号, htons()関数は16bitホストバイトオーダーをネットワークバイトオーダーに変換
        addr.sin_addr.s_addr = dst_ip; // IPアドレス

        // データ送信
        sendto(sockfd, message, now_length, 0, (struct sockaddr *)&addr, sizeof(addr)); 
    }
}

inline void ARWS::sendMessageFunc(RandomWalker& RWer, const uint32_t& dst_ip, int sockfd, const uint16_t& port_num) {
    // アドレスの生成
    struct sockaddr_in addr; // 接続先の情報用の構造体(ipv4)
    memset(&addr, 0, sizeof(struct sockaddr_in)); // memsetで初期化
    addr.sin_family = AF_INET; // アドレスファミリ(ipv4)
    addr.sin_port = htons(port_num); // ポート番号, htons()関数は16bitホストバイトオーダーをネットワークバイトオーダーに変換
    addr.sin_addr.s_addr = dst_ip; // IPアドレス

    // データ送信
    sendto(sockfd, &RWer, MESSAGE_MAX_LENGTH, 0, (struct sockaddr *)&addr, sizeof(addr)); 
}

inline void ARWS::receiveMessage(int sockfd, const uint16_t& port_num) {
    std::cout << "receive_message: " << port_num << std::endl;

    while (1) {
        // messageを受信
        std::unique_ptr<char[]> message(new char[MESSAGE_MAX_LENGTH + sizeof(MESSAGE_END)]);
        memset(message.get(), 0, MESSAGE_MAX_LENGTH + sizeof(MESSAGE_END));
        recv(sockfd, message.get(), MESSAGE_MAX_LENGTH + sizeof(MESSAGE_END), 0);

        // // デバッグ
        // std::cout << message.get() << std::endl;

        uint32_t* message_id = (uint32_t*)(message.get());

        if (*message_id == 1) { // 実験開始の合図

            uint32_t* ip = (uint32_t*)(message.get() + sizeof(uint32_t));
            uint32_t* num_RWer = (uint32_t*)(message.get() + sizeof(uint32_t) + sizeof(uint32_t));

            startmanagerip_ = *ip;
            RW_config_.setNumberOfRWExecution(*num_RWer);

            // debug
            std::cout << *num_RWer << std::endl;

            // 実験開始のフラグを立てる
            start_flag_.writeReady(true);

        } else if (*message_id == 2) { // RWer のメッセージ
            // RWer を一つずつ分ける
            uint32_t stream_length = 0;

            while (1) {
                if (*message_id == MESSAGE_END) break;

                // std::unique_ptr<RandomWalker> RWer_ptr((RandomWalker*)(message.get() + stream_length));
                std::unique_ptr<RandomWalker> RWer_ptr = std::make_unique<RandomWalker>(*((RandomWalker*)(message.get() + stream_length)));

                uint32_t RWer_data_length = RWer_ptr->getSize();

                // メッセージキューに push
                receive_queue_[port_num].push(std::move(RWer_ptr));

                stream_length += RWer_data_length;
                message_id = (uint32_t*)(message.get() + stream_length); // 次の message_id を特定
            }

        } else if (*message_id == 3) { // 実験結果を送信

            sendToStartManager();

        } else {
            perror("wrong id");
            exit(1); // 異常終了
        }

    }
}

inline int ARWS::createUdpServerSocket(const uint16_t& port_num) {
    // ソケットの生成
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { // エラー処理
        perror("socket");
        exit(1); // 異常終了
    }

    // アドレスの生成
    struct sockaddr_in addr; // 接続先の情報用の構造体(ipv4)
    memset(&addr, 0, sizeof(struct sockaddr_in)); // memsetで初期化
    addr.sin_family = AF_INET; // アドレスファミリ(ipv4)
    addr.sin_port = htons(port_num); // ポート番号, htons()関数は16bitホストバイトオーダーをネットワークバイトオーダーに変換
    addr.sin_addr.s_addr = hostip_; // IPアドレス, inet_addr()関数はアドレスの翻訳

    // ソケット登録
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { // ソケット, アドレスポインタ, アドレスサイズ // エラー処理
        perror("bind");
        exit(1); // 異常終了
    }

    return sockfd;
}

inline int ARWS::createTcpServerSocket(const uint16_t& port_num) {
    // ソケットの生成
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { // エラー処理
        perror("socket");
        exit(1); // 異常終了
    }

    // アドレスの生成
    struct sockaddr_in addr; // 接続先の情報用の構造体(ipv4)
    memset(&addr, 0, sizeof(struct sockaddr_in)); // memsetで初期化
    addr.sin_family = AF_INET; // アドレスファミリ(ipv4)
    addr.sin_port = htons(port_num); // ポート番号, htons()関数は16bitホストバイトオーダーをネットワークバイトオーダーに変換
    addr.sin_addr.s_addr = hostip_; // IPアドレス, inet_addr()関数はアドレスの翻訳

    // ソケット登録
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { // ソケット, アドレスポインタ, アドレスサイズ // エラー処理
        perror("bind");
        exit(1); // 異常終了
    } else {
        std::cout << "bind ok" << std::endl;
    }

    // 受信待ち
    if (listen(sockfd, SOMAXCONN) < 0) { // ソケット, キューの最大長 // エラー処理
        perror("listen");
        close(sockfd); // ソケットクローズ
        exit(1); // 異常終了
    } else {
        std::cout << "listen ok" << std::endl;    
    }

    return sockfd;
}

inline void ARWS::sendToStartManager() {
    // observer 停止
    observer_flag_ = false;
    for (std::thread &th : observer_thread_) {
        th.join();
    }
    observer_thread_.clear();

    // start manager に送信するのは, RW 終了数, 実行時間
    uint32_t end_count = RW_manager_.getEndcnt();
    double execution_time = RW_manager_.getExecutionTime();
    std::cout << "end_count : " << end_count << std::endl;
    std::cout << "execution_time : " << execution_time << std::endl;

    // デバッグ
    std::cout << "message_queue_size: " << receive_queue_.size() << std::endl;
    for (auto& [port, q] : receive_queue_) {
        std::cout << port << ": " << q.getSize() << std::endl;
    }
    std::cout << "send_queue_size: " << send_queue_.size() << std::endl;
    for (auto& [ip, q] : send_queue_) {
        std::cout << ip << ": " << q.getSize() << std::endl;   
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    {
        // ソケットの生成
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) { // エラー処理
            perror("socket");
            exit(1); // 異常終了
        }

        // アドレスの生成
        struct sockaddr_in addr; // 接続先の情報用の構造体(ipv4)
        memset(&addr, 0, sizeof(struct sockaddr_in)); // memsetで初期化
        addr.sin_family = AF_INET; // アドレスファミリ(ipv4)
        addr.sin_port = htons(9999); // ポート番号, htons()関数は16bitホストバイトオーダーをネットワークバイトオーダーに変換
        addr.sin_addr.s_addr = startmanagerip_; // IPアドレス, inet_addr()関数はアドレスの翻訳

        // ソケット接続要求
        connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)); // ソケット, アドレスポインタ, アドレスサイズ

        // データ送信 (hostip: 4B, end_count: 4B, all_execution_time: 8B)
        char message[1024];
        memcpy(message, &hostip_, sizeof(uint32_t));
        memcpy(message + sizeof(uint32_t), &end_count, sizeof(uint32_t));
        memcpy(message + sizeof(uint32_t) + sizeof(uint32_t), &execution_time, sizeof(double));
        send(sockfd, message, sizeof(message), 0); // 送信

        // ソケットクローズ
        close(sockfd); 
    }

    // RW Manager　のリセット
    RW_manager_.reset();

    std::cout << "reset ok" << std::endl;
}

inline void ARWS::observer() {
    while (observer_flag_) {
        RW_manager_.printSurvivingRWerNum();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}