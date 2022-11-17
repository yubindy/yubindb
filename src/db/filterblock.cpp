#include "filterblock.h"
namespace yubindb {
FilterBlockbuilder::FilterBlockbuilder() {}
void FilterBlockbuilder::StartBlock(uint64_t block_offset) {}
void FilterBlockbuilder::AddKey(const std::string_view& key) {}
std::string_view FilterBlockbuilder::Finish() {}
FilterBlockreader::FilterBlockreader(const std::string_view& contents){}
bool FilterBlockreader::KeyMayMatch(uint64_t block_offset,
                                    const std::string_view& key){}
};  // namespace yubindb