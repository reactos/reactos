
#define CHANGES_TO_MERGE_WITH_WINERROR_H

//
//  please place any new messages (that have NOT been send to both
//  Alan & Bryan) in the following define:
//
//  when adding to this file, PLEASE update the following "nexts":
//
#define CRYPT_NEXT_0x8009_ERROR_CODE_AVAILABLE      0x202C
#define CRYPT_NEXT_0x800B_ERROR_CODE_AVAILABLE      0x0110

#ifdef CHANGES_TO_MERGE_WITH_WINERROR_H




//
// MessageId: CERT_E_CN_NO_MATCH
//
// MessageText:
//
//  The certificate's CN name does not match the passed value.
//
#define CERT_E_CN_NO_MATCH              _HRESULT_TYPEDEF_(0x800B010FL)

//
// MessageId: CERT_E_UNTRUSTEDTESTROOT
//
// MessageText:
//
//  The root certificate is a testing certificate and the policy settings disallow test certificates
//
#define CERT_E_UNTRUSTEDTESTROOT        _HRESULT_TYPEDEF_(0x800B010DL)


#endif // CHANGES_TO_MERGE_WITH_WINERROR_H

// #ifdef <error define>
// #   ifdef _SHOW_WINCRERR_MSG
// #        pragma message("WINCRERR.H: time to clean up section (CRYPT_E_FILERESIZED)")
// #   endif
// #else
// #    define  <error define>
// #endif // <error define>


#ifdef TRUST_E_SYSTEM_ERROR

#   ifdef _SHOW_WINCRERR_MSG
#       pragma message("WINCRERR.H: time to clean up section TRUST_E_SYSTEM_ERROR")
#   endif

#else

//
//  CERT_E_VALIDIYPERIODNESTING -- MISSPELLING!!!!
//
//
// MessageId: CERT_E_VALIDITYPERIODNESTING
//
// MessageText:
//
//  The validity periods of the certification chain do not nest correctly.
//
#define CERT_E_VALIDITYPERIODNESTING                    _HRESULT_TYPEDEF_(0x800B0102L)


//
// MessageId: TRUST_E_SYSTEM_ERROR
//
// MessageText:
//
//  A system level error occured
//
#define TRUST_E_SYSTEM_ERROR                            _HRESULT_TYPEDEF_(0x80096001L)

//
// MessageId: TRUST_E_SYSTEM_ERROR
//
// MessageText:
//
//  The cert for the signer of the message is invalid or not found
//
#define TRUST_E_NO_SIGNER_CERT                          _HRESULT_TYPEDEF_(0x80096002L)

//
// MessageId: TRUST_E_COUNTER_SIGNER
//
// MessageText:
//
//  One of the counter signers was invalid
//
#define TRUST_E_COUNTER_SIGNER                          _HRESULT_TYPEDEF_(0x80096003L)

//
// MessageId: TRUST_E_CERT_SIGNATURE
//
// MessageText:
//
//  The signature of the certificate can not be validated
//
#define TRUST_E_CERT_SIGNATURE                          _HRESULT_TYPEDEF_(0x80096004L)

// MessageId: TRUST_E_TIME_STAMP
//
// MessageText:
//
//  One of the counter signers was invalid
//
#define TRUST_E_TIME_STAMP                              _HRESULT_TYPEDEF_(0x80096005L)

//
// MessageId: TRUST_E_BAD_DIGEST
//
// MessageText:
//
//  The objects digest did not verify
//
#define TRUST_E_BAD_DIGEST                              _HRESULT_TYPEDEF_(0x80096010L)


//
// MessageId: TRUST_E_BASIC_CONSTRAINTS
//
// MessageText:
//
//  The Certificates Basic Constraints are invalid
//
#define TRUST_E_BASIC_CONSTRAINTS                       _HRESULT_TYPEDEF_(0x80096019L)

//
// MessageId: TRUST_E_FINANCIAL_CRITERIA
//
// MessageText:
//
//  Certificate does not meet/contain the Authenticode Financial Extensions.
//
#define TRUST_E_FINANCIAL_CRITERIA                      _HRESULT_TYPEDEF_(0x8009601EL)

//
// MessageId: CERT_E_REVOCATION_FAILURE
//
// MessageText:
//
//  The revocation process had an error - the certificate could not be checked.
//
#define CERT_E_REVOCATION_FAILURE                       _HRESULT_TYPEDEF_(0x800B010EL)


#endif // TRUST_E_SYSTEM_ERROR


#ifdef CRYPT_E_VERIFY_USAGE_OFFLINE

#   ifdef _SHOW_WINCRERR_MSG
#       pragma message("WINCRERR.H: time to clean up section CRYPT_E_VERIFY_USAGE_OFFLINE")
#   endif

#else
//
// MessageId: CRYPT_E_VERIFY_USAGE_OFFLINE
//
// MessageText:
//
//  Since the server was offline, the called function wasn't able to
//  complete the usage check.
//
#define CRYPT_E_VERIFY_USAGE_OFFLINE        _HRESULT_TYPEDEF_(0x80092029L)

#endif // CRYPT_E_VERIFY_USAGE_OFFLINE

#ifdef CRYPT_E_NO_VERIFY_USAGE_DLL

#   ifdef _SHOW_WINCRERR_MSG
#       pragma message("WINCRERR.H: time to clean up section CRYPT_E_NO_VERIFY_USAGE_DLL")
#   endif

#else

//
// MessageId: CRYPT_E_NO_VERIFY_USAGE_DLL
//
// MessageText:
//
//  No Dll or exported function was found to verify subject usage.
//
#define CRYPT_E_NO_VERIFY_USAGE_DLL         _HRESULT_TYPEDEF_(0x80092027L)

//
// MessageId: CRYPT_E_NO_VERIFY_USAGE_CHECK
//
// MessageText:
//
//  The called function wasn't able to do a usage check on the subject
//
#define CRYPT_E_NO_VERIFY_USAGE_CHECK       _HRESULT_TYPEDEF_(0x80092028L)

//
// MessageId: CRYPT_E_NOT_IN_CTL
//
// MessageText:
//
//  The subject wasn't found in a Certificate Trust List (CTL).
//
#define CRYPT_E_NOT_IN_CTL                  _HRESULT_TYPEDEF_(0x8009202AL)

//
// MessageId: CRYPT_E_NO_TRUSTED_SIGNER
//
// MessageText:
//
//  No trusted signer was found to verify the signature of the message or
//  trust list.
//
#define CRYPT_E_NO_TRUSTED_SIGNER           _HRESULT_TYPEDEF_(0x8009202BL)


#endif //  CRYPT_E_NO_VERIFY_USAGE_DLL


#ifdef CRYPT_E_SECURITY_SETTINGS

#   ifdef _SHOW_WINCRERR_MSG
#       pragma message("WINCRERR.H: time to clean up section CRYPT_E_SECURITY_SETTINGS")
#   endif

#else

//
// MessageId: CRYPT_E_SECURITY_SETTINGS
//
// MessageText:
//
//  The cryptography operation has failed due to a Security option setting.  This
//  may be a setting in IE, or, using the SETREG.EXE utility.
//
#define CRYPT_E_SECURITY_SETTINGS       _HRESULT_TYPEDEF_(0x80092026L)

#endif // CRYPT_E_SECUTIY_SETTINGS


#ifndef CRYPT_E_REVOKED
#define CRYPT_E_REVOKED             _HRESULT_TYPEDEF_(0x80092010L)
#endif

//
//  SP4
//
#ifndef CRYPT_E_FILERESIZED

#define CRYPT_E_FILERESIZED                     _HRESULT_TYPEDEF_(0x80092025L)
#define CRYPT_E_NOT_IN_REVOCATION_DATABASE      _HRESULT_TYPEDEF_(0x80092014L)
#define CRYPT_E_SECURITY_SETTINGS               _HRESULT_TYPEDEF_(0x80092026L)
#define CRYPT_E_NO_VERIFY_USAGE_DLL             _HRESULT_TYPEDEF_(0x80092027L)
#define CRYPT_E_NO_VERIFY_USAGE_CHECK           _HRESULT_TYPEDEF_(0x80092028L)
#define CRYPT_E_VERIFY_USAGE_OFFLINE            _HRESULT_TYPEDEF_(0x80092029L)
#define CRYPT_E_NOT_IN_CTL                      _HRESULT_TYPEDEF_(0x8009202AL)
#define CRYPT_E_NO_TRUSTED_SIGNER               _HRESULT_TYPEDEF_(0x8009202BL)
#define CRYPT_E_STREAM_MSG_NOT_READY            _HRESULT_TYPEDEF_(0x80091010L)
#define CRYPT_E_STREAM_INSUFFICIENT_DATA        _HRESULT_TYPEDEF_(0x80091011L)
#define CRYPT_E_INVALID_X500_STRING             _HRESULT_TYPEDEF_(0x80092023L)
#define CRYPT_E_NOT_CHAR_STRING                 _HRESULT_TYPEDEF_(0x80092024L)

#define TRUST_E_SYSTEM_ERROR                    _HRESULT_TYPEDEF_(0x80096001L)
#define TRUST_E_NO_SIGNER_CERT                  _HRESULT_TYPEDEF_(0x80096002L)
#define TRUST_E_COUNTER_SIGNER                  _HRESULT_TYPEDEF_(0x80096003L)
#define TRUST_E_CERT_SIGNATURE                  _HRESULT_TYPEDEF_(0x80096004L)
#define TRUST_E_TIME_STAMP                      _HRESULT_TYPEDEF_(0x80096005L)
#define TRUST_E_BAD_DIGEST                      _HRESULT_TYPEDEF_(0x80096010L)
#define TRUST_E_BASIC_CONSTRAINTS               _HRESULT_TYPEDEF_(0x80096019L)
#define TRUST_E_FINANCIAL_CRITERIA              _HRESULT_TYPEDEF_(0x8009601EL)

#define WIN_CERT_REVISION_2_0                   (0x0200)

#pragma warning(disable: 4245)

#endif // SP4
