

#ifndef QLINK_H_
#define QLINK_H_

// Length of the text under each quick links
#define MAX_QL_TEXT_LENGTH      256
#define MAX_QL_WIDTH            92


HRESULT CQuickLinks_CreateInstance(IDeskBand **ppunk);
#define QLCMD_SINGLELINE 1

#endif // QLINK_H_

