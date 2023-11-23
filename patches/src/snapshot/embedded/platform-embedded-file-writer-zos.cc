// Copyright 2023 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/snapshot/embedded/platform-embedded-file-writer-zos.h"

#include <stdarg.h>

#include <string>

namespace v8 {
namespace internal {

// https://www.ibm.com/docs/en/zos/2.1.0?topic=conventions-continuation-lines
// for length of HLASM statements and continuation.
static constexpr int __kAsmMaxLineLen = 71;
static constexpr int __kAsmContIndentLen = 15;
static constexpr int __kAsmContMaxLen = __kAsmMaxLineLen - __kAsmContIndentLen;

// https://www.ibm.com/docs/sl/zos/2.1.0?topic=extensions-symbol-length
static constexpr int __kAsmSymbolMaxLen = 63;

namespace {
int hlasm_fprintf_line(FILE* fp, const char* fmt, ...) {
  int ret;
  char buffer[4096];
  int offset = 0;
  static char indent[__kAsmContIndentLen] = "";
  va_list ap;
  va_start(ap, fmt);
  ret = vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);
  if (!*indent)
    memset(indent, ' ', sizeof(indent));
  if (ret > __kAsmMaxLineLen && buffer[__kAsmMaxLineLen] != '\n') {
    offset += fwrite(buffer + offset, 1, __kAsmMaxLineLen, fp);
    // Write continuation mark
    fwrite("-\n", 1, 2, fp);
    ret -= __kAsmMaxLineLen;
    while (ret > __kAsmContMaxLen) {
      // indent by __kAsmContIndentLen
      fwrite(indent, 1, __kAsmContIndentLen, fp);
      offset += fwrite(buffer + offset, 1, __kAsmContMaxLen, fp);
      // write continuation mark
      fwrite("-\n", 1, 2, fp);
      ret -= __kAsmContMaxLen;
    }
    if (ret > 0) {
      // indent __kAsmContIndentLen blanks
      fwrite(indent, 1, __kAsmContIndentLen, fp);
      offset += fwrite(buffer + offset, 1, ret, fp);
    }
  } else {
    offset += fwrite(buffer + offset, 1, ret, fp);
  }
  return ret;
}
}  // namespace

void DeclareLabelProlog(FILE *fp, const char* name) {
  fprintf(fp,
          "&suffix SETA &suffix+1\n"
          "CEECWSA LOCTR\n"
          "AL&suffix ALIAS C'%s'\n"
          "C_WSA64 CATTR DEFLOAD,RMODE(64),PART(AL&suffix)\n"
          "AL&suffix XATTR REF(DATA),LINKAGE(XPLINK),SCOPE(EXPORT)\n",
          name);
}

void DeclareLabelEpilogue(FILE *fp) {
  fprintf(fp,
          "C_WSA64 CATTR PART(PART1)\n"
          "LBL&suffix DC AD(AL&suffix)\n");
}

void PlatformEmbeddedFileWriterZOS::DeclareUint32(const char* name,
                                                  uint32_t value) {
  DeclareSymbolGlobal(name);
  fprintf(fp_,
          "&suffix SETA &suffix+1\n"
          "CEECWSA LOCTR\n"
          "AL&suffix ALIAS C'%s'\n"
          "C_WSA64 CATTR DEFLOAD,RMODE(64),PART(AL&suffix)\n"
          "AL&suffix XATTR REF(DATA),LINKAGE(XPLINK),SCOPE(EXPORT)\n"
          " DC F'%d'\n"
          "C_WSA64 CATTR PART(PART1)\n"
          "LBL&suffix DC AD(AL&suffix)\n",
          name, value);
}

void PlatformEmbeddedFileWriterZOS::DeclareSymbolGlobal(const char* name) {
  hlasm_fprintf_line(fp_, "* Global Symbol %s\n", name);
}

void PlatformEmbeddedFileWriterZOS::AlignToCodeAlignment() {
  // No code alignment required.
}

void PlatformEmbeddedFileWriterZOS::AlignToDataAlignment() {
  // No data alignment required.
}

void PlatformEmbeddedFileWriterZOS::Comment(const char* string) {
  hlasm_fprintf_line(fp_, "* %s\n", string);
}

void PlatformEmbeddedFileWriterZOS::DeclareLabel(const char* name) {
  hlasm_fprintf_line(fp_, "*--------------------------------------------\n");
  hlasm_fprintf_line(fp_, "* Label %s\n", name);
  hlasm_fprintf_line(fp_, "*--------------------------------------------\n");
  hlasm_fprintf_line(fp_, "%s DS 0H\n", name);
}

void PlatformEmbeddedFileWriterZOS::SourceInfo(int fileid, const char* filename,
                                               int line) {
  hlasm_fprintf_line(fp_, "* line %d \"%s\"\n", line, filename);
}

void PlatformEmbeddedFileWriterZOS::DeclareFunctionBegin(const char* name,
                                                         uint32_t size) {
  hlasm_fprintf_line(fp_, "*--------------------------------------------\n");
  hlasm_fprintf_line(fp_, "* Builtin %s\n", name);
  hlasm_fprintf_line(fp_, "*--------------------------------------------\n");
  int len = strlen(name);
  if (len > __kAsmSymbolMaxLen) {
    char newname[__kAsmSymbolMaxLen + 1];
    newname[sizeof(newname) - 1] = 0;
    strcpy(newname, "_bi_");
    // + 5: for _bi_ and terminator '\0'
    memcpy(newname + 4, name + len - sizeof(newname) + 5, sizeof(newname) - 5);
    hlasm_fprintf_line(fp_, "%s DS 0H\n", newname);
  } else {
    hlasm_fprintf_line(fp_, "%s DS 0H\n", name);
  }
}

void PlatformEmbeddedFileWriterZOS::DeclareFunctionEnd(const char* name) {
  // Not used.
}

int PlatformEmbeddedFileWriterZOS::HexLiteral(uint64_t value) {
  return fprintf(fp_, "%.16lx", value);
}

void PlatformEmbeddedFileWriterZOS::FilePrologue() {
  fprintf(fp_,
          "&C SETC 'embed'\n"
          " SYSSTATE AMODE64=YES\n"
          "&C csect\n"
          "&C amode 64\n"
          "&C rmode 64\n");
}

void PlatformEmbeddedFileWriterZOS::DeclareExternalFilename(
    int fileid, const char* filename) {
  // Not used.
}

void PlatformEmbeddedFileWriterZOS::FileEpilogue() { fprintf(fp_, " end\n"); }

int PlatformEmbeddedFileWriterZOS::IndentedDataDirective(
    DataDirective directive) {
  // Not used.
  return 0;
}

DataDirective PlatformEmbeddedFileWriterZOS::ByteChunkDataDirective() const {
  return kQuad;
}

int PlatformEmbeddedFileWriterZOS::WriteByteChunk(const uint8_t* data) {
  DCHECK_EQ(ByteChunkDataDirective(), kQuad);
  const uint64_t* quad_ptr = reinterpret_cast<const uint64_t*>(data);
  return HexLiteral(*quad_ptr);
}

void PlatformEmbeddedFileWriterZOS::SectionText() {
  // Not used.
}

void PlatformEmbeddedFileWriterZOS::SectionRoData() {
  // Not used.
}

}  // namespace internal
}  // namespace v8
