// #define SPDLOG_NAME "SmartDispenser"
// #define SPDLOG_ACTIVE_LEVEL \
//   SPDLOG_LEVEL_TRACE  //必须定义这个宏,才能输出文件名和行号
// #include <spdlog/sinks/rotating_file_sink.h>
// #include <spdlog/spdlog.h>

// bool initLog(void) {
//   //初始化日志spdlog,https://github.com/gabime/spdlog
// //   CString strFilePath = FILEMANAGE->GetLogsDir() + _T("\\logApp.txt");
// //   std::string logpath = CT2A(strFilePath.GetBuffer());
// //   strFilePath.ReleaseBuffer();

//   try {
//     auto rotating_logger =
//         spdlog::rotating_logger_mt(SPDLOG_NAME, logpath, 1024 * 1024 * 1, 3);
//     spdlog::set_default_logger(rotating_logger);
//     rotating_logger->set_level(spdlog::level::debug);
//     //输出格式请参考https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
//     rotating_logger->set_pattern(
//         "[%Y-%m-%d %H:%M:%S.%e][thread %t][%@,%!][%l] : %v");
//   } catch (const spdlog::spdlog_ex& ex) {
//     std::cout << "Log initialization failed: " << ex.what() << std::endl;
//     CString info;
//     info.Format(_T("log init failed: %s\n"), ex.what());
//     addInitInfo(theApp.MyMsgString(_T("日志初始化失败!"), info));
//     return false;
//   }

//   return true;
// }