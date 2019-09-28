// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/cert/internal/general_names.h"

#include "base/strings/string_util.h"
#include "net/cert/internal/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

::testing::AssertionResult LoadTestData(const char* token,
                                        const std::string& basename,
                                        std::string* result) {
  std::string path = "net/data/name_constraints_unittest/" + basename;

  const PemBlockMapping mappings[] = {
      {token, result},
  };

  return ReadTestDataFromPemFile(path, mappings);
}

::testing::AssertionResult LoadTestSubjectAltNameData(
    const std::string& basename,
    std::string* result) {
  return LoadTestData("SUBJECT ALTERNATIVE NAME", basename, result);
}

}  // namespace

TEST(GeneralNames, CreateFailsOnEmptySubjectAltName) {
  std::string invalid_san_der;
  ASSERT_TRUE(
      LoadTestSubjectAltNameData("san-invalid-empty.pem", &invalid_san_der));
  CertErrors errors;
  EXPECT_FALSE(GeneralNames::Create(der::Input(&invalid_san_der), &errors));
}

TEST(GeneralNames, OtherName) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-othername.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_OTHER_NAME, general_names->present_name_types);
  const uint8_t expected_der[] = {0x06, 0x04, 0x2a, 0x03, 0x04, 0x05,
                                  0x04, 0x04, 0xde, 0xad, 0xbe, 0xef};
  ASSERT_EQ(1U, general_names->other_names.size());
  EXPECT_EQ(der::Input(expected_der), general_names->other_names[0]);
}

TEST(GeneralNames, RFC822Name) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-rfc822name.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_RFC822_NAME, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->rfc822_names.size());
  EXPECT_EQ("foo@example.com", general_names->rfc822_names[0]);
}

TEST(GeneralNames, CreateFailsOnNonAsciiRFC822Name) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-rfc822name.pem", &san_der));
  base::ReplaceFirstSubstringAfterOffset(&san_der, 0, "foo@example.com",
                                         "f\xF6\xF6@example.com");
  CertErrors errors;
  EXPECT_FALSE(GeneralNames::Create(der::Input(&san_der), &errors));
}

TEST(GeneralNames, DnsName) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-dnsname.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_DNS_NAME, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->dns_names.size());
  EXPECT_EQ("foo.example.com", general_names->dns_names[0]);
}

TEST(GeneralNames, CreateFailsOnNonAsciiDnsName) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-dnsname.pem", &san_der));
  base::ReplaceFirstSubstringAfterOffset(&san_der, 0, "foo.example.com",
                                         "f\xF6\xF6.example.com");
  CertErrors errors;
  EXPECT_FALSE(GeneralNames::Create(der::Input(&san_der), &errors));
}

TEST(GeneralNames, X400Address) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-x400address.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_X400_ADDRESS, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->x400_addresses.size());
  const uint8_t expected_der[] = {0x30, 0x06, 0x61, 0x04,
                                  0x13, 0x02, 0x55, 0x53};
  EXPECT_EQ(der::Input(expected_der), general_names->x400_addresses[0]);
}

TEST(GeneralNames, DirectoryName) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-directoryname.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_DIRECTORY_NAME, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->directory_names.size());
  const uint8_t expected_der[] = {0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55,
                                  0x04, 0x06, 0x13, 0x02, 0x55, 0x53};
  EXPECT_EQ(der::Input(expected_der), general_names->directory_names[0]);
}

TEST(GeneralNames, EDIPartyName) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-edipartyname.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_EDI_PARTY_NAME, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->edi_party_names.size());
  const uint8_t expected_der[] = {0x81, 0x03, 0x66, 0x6f, 0x6f};
  EXPECT_EQ(der::Input(expected_der), general_names->edi_party_names[0]);
}

TEST(GeneralNames, URI) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-uri.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_UNIFORM_RESOURCE_IDENTIFIER,
            general_names->present_name_types);
  ASSERT_EQ(1U, general_names->uniform_resource_identifiers.size());
  EXPECT_EQ("http://example.com",
            general_names->uniform_resource_identifiers[0]);
}

TEST(GeneralNames, CreateFailsOnNonAsciiURI) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-uri.pem", &san_der));
  base::ReplaceFirstSubstringAfterOffset(&san_der, 0, "http://example.com",
                                         "http://ex\xE4mple.com");
  CertErrors errors;
  EXPECT_FALSE(GeneralNames::Create(der::Input(&san_der), &errors));
}

TEST(GeneralNames, IPAddress_v4) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-ipaddress4.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_IP_ADDRESS, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->ip_addresses.size());
  EXPECT_EQ(IPAddress(192, 168, 6, 7), general_names->ip_addresses[0]);
  EXPECT_EQ(0U, general_names->ip_address_ranges.size());
}

TEST(GeneralNames, IPAddress_v6) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-ipaddress6.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_IP_ADDRESS, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->ip_addresses.size());
  EXPECT_EQ(
      IPAddress(0xFE, 0x80, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14),
      general_names->ip_addresses[0]);
  EXPECT_EQ(0U, general_names->ip_address_ranges.size());
}

TEST(GeneralNames, CreateFailsOnInvalidLengthIpAddress) {
  std::string invalid_san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-invalid-ipaddress.pem",
                                         &invalid_san_der));
  CertErrors errors;
  EXPECT_FALSE(GeneralNames::Create(der::Input(&invalid_san_der), &errors));
}

TEST(GeneralNames, RegisteredIDs) {
  std::string san_der;
  ASSERT_TRUE(LoadTestSubjectAltNameData("san-registeredid.pem", &san_der));

  CertErrors errors;
  std::unique_ptr<GeneralNames> general_names =
      GeneralNames::Create(der::Input(&san_der), &errors);
  ASSERT_TRUE(general_names);
  EXPECT_EQ(GENERAL_NAME_REGISTERED_ID, general_names->present_name_types);
  ASSERT_EQ(1U, general_names->registered_ids.size());
  const uint8_t expected_der[] = {0x2a, 0x03, 0x04};
  EXPECT_EQ(der::Input(expected_der), general_names->registered_ids[0]);
}

}  // namespace net
