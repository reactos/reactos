/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

	atm.h

Abstract:

	This module defines the structures, macros, and manifests available
	to ATM aware components.

Author:

	Jameel Hyder - jameelh@microsoft.com

Revision History:

	Initial Version - March 1996
#ifdef MS_UNI4
	John Dalgas (v-jdalga)	09/18/97		added support for UNI4.0 ATM CM
#endif // MS_UNI4

--*/

#ifndef	_ATM_H_
#define	_ATM_H_

//
// Address type
//
typedef ULONG	ATM_ADDRESSTYPE;

#define	ATM_NSAP				0
#define	ATM_E164				1

//
// ATM Address
//
#define	ATM_MAC_ADDRESS_LENGTH	6		// Same as 802.x
#define	ATM_ADDRESS_LENGTH		20

//
//  Special characters in ATM address string used in textual representations
//
#define ATM_ADDR_BLANK_CHAR				L' '
#define ATM_ADDR_PUNCTUATION_CHAR		L'.'
#define ATM_ADDR_E164_START_CHAR		L'+'

typedef struct _ATM_ADDRESS
{
	ATM_ADDRESSTYPE				AddressType;
	ULONG						NumberOfDigits;
	UCHAR						Address[ATM_ADDRESS_LENGTH];
} ATM_ADDRESS, *PATM_ADDRESS;



//
// AAL types that the miniport supports
//
#define	AAL_TYPE_AAL0			1
#define	AAL_TYPE_AAL1			2
#define	AAL_TYPE_AAL34			4
#define	AAL_TYPE_AAL5			8

typedef ULONG	ATM_AAL_TYPE, *PATM_AAL_TYPE;


//
// Flags for Information Element types
//
#define ATM_IE_RESPONSE_FLAG		0x80000000
#define ATM_IE_EMPTY_FLAG			0x40000000


//
// Types of Information Elements
//
typedef enum
{
	// These identify request IEs
	IE_AALParameters,
	IE_TrafficDescriptor,
	IE_BroadbandBearerCapability,
	IE_BHLI,
	IE_BLLI,
	IE_CalledPartyNumber,
	IE_CalledPartySubaddress,
	IE_CallingPartyNumber,
	IE_CallingPartySubaddress,
	IE_Cause,
	IE_QOSClass,
	IE_TransitNetworkSelection,
	IE_BroadbandSendingComplete,
	IE_LIJCallId,
	IE_Raw,
#ifdef MS_UNI4
	IE_TrafficDescriptor_AddOn,
	IE_BroadbandBearerCapability_Uni40,
	IE_BLLI_AddOn,
	IE_ConnectionId,
	IE_NotificationIndicator,
	IE_MinimumTrafficDescriptor,
	IE_AlternativeTrafficDescriptor,
	IE_ExtendedQOS,
	IE_EndToEndTransitDelay,
	IE_ABRSetupParameters,
	IE_ABRAdditionalParameters,
	IE_LIJParameters,
	IE_LeafSequenceNumber,
	IE_ConnectionScopeSelection,
	IE_UserUser,
	IE_GenericIDTransport,
	IE_ConnectedNumber,					// invalid to use in request
	IE_ConnectedSubaddress,         	// invalid to use in request
#endif // MS_UNI4
	// End of request IEs
	IE_NextRequest,						// invalid to use in request

	// These identify empty IE buffers, to hold a possible response
	IE_Cause_Empty						= IE_Cause			   		| ATM_IE_EMPTY_FLAG,
#ifdef MS_UNI4
	IE_ConnectionId_Empty				= IE_ConnectionId			| ATM_IE_EMPTY_FLAG,
	IE_ConnectedNumber_Empty			= IE_ConnectedNumber		| ATM_IE_EMPTY_FLAG,
	IE_ConnectedSubaddress_Empty		= IE_ConnectedSubaddress	| ATM_IE_EMPTY_FLAG,
	IE_NotificationIndicator_Empty		= IE_NotificationIndicator	| ATM_IE_EMPTY_FLAG,
	IE_UserUser_Empty					= IE_UserUser				| ATM_IE_EMPTY_FLAG,
	IE_GenericIDTransport_Empty			= IE_GenericIDTransport		| ATM_IE_EMPTY_FLAG,
#endif // MS_UNI4

	// These identify response IEs
	IE_Cause_Response					= IE_Cause					| ATM_IE_RESPONSE_FLAG,
	IE_AALParameters_Response			= IE_AALParameters			| ATM_IE_RESPONSE_FLAG,
	IE_BLLI_Response					= IE_BLLI					| ATM_IE_RESPONSE_FLAG,
#ifdef MS_UNI4
	IE_BLLI_AddOn_Response				= IE_BLLI_AddOn			   	| ATM_IE_RESPONSE_FLAG,
	IE_TrafficDescriptor_Response		= IE_TrafficDescriptor		| ATM_IE_RESPONSE_FLAG,
	IE_TrafficDescriptor_AddOn_Response	= IE_TrafficDescriptor_AddOn| ATM_IE_RESPONSE_FLAG,
	IE_ConnectionId_Response			= IE_ConnectionId			| ATM_IE_RESPONSE_FLAG,
	IE_NotificationIndicator_Response	= IE_NotificationIndicator	| ATM_IE_RESPONSE_FLAG,
	IE_ExtendedQOS_Response				= IE_ExtendedQOS			| ATM_IE_RESPONSE_FLAG,
	IE_EndToEndTransitDelay_Response	= IE_EndToEndTransitDelay	| ATM_IE_RESPONSE_FLAG,
	IE_ABRSetupParameters_Response		= IE_ABRSetupParameters		| ATM_IE_RESPONSE_FLAG,
	IE_ABRAdditionalParameters_Response	= IE_ABRAdditionalParameters| ATM_IE_RESPONSE_FLAG,
	IE_ConnectedNumber_Response			= IE_ConnectedNumber		| ATM_IE_RESPONSE_FLAG,
	IE_ConnectedSubaddress_Response		= IE_ConnectedSubaddress	| ATM_IE_RESPONSE_FLAG,
	IE_UserUser_Response				= IE_UserUser				| ATM_IE_RESPONSE_FLAG,
	IE_GenericIDTransport_Response		= IE_GenericIDTransport		| ATM_IE_RESPONSE_FLAG,
#endif // MS_UNI4
} Q2931_IE_TYPE;



//
// Common header for each Information Element
//
typedef struct _Q2931_IE
{
	Q2931_IE_TYPE				IEType;
	ULONG						IELength;	// Bytes, including IEType and IELength fields
	UCHAR						IE[1];
} Q2931_IE, *PQ2931_IE;


//
// Definitions for SapType in CO_SAP
//
#define SAP_TYPE_NSAP			0x00000001
#define SAP_TYPE_E164			0x00000002

//
// Values used for the Mode field in AAL5_PARAMETERS
//
#define AAL5_MODE_MESSAGE			0x01
#define AAL5_MODE_STREAMING			0x02

//
// Values used for the SSCSType field in AAL5_PARAMETERS
//
#define AAL5_SSCS_NULL				0x00
#define AAL5_SSCS_SSCOP_ASSURED		0x01
#define AAL5_SSCS_SSCOP_NON_ASSURED	0x02
#define AAL5_SSCS_FRAME_RELAY		0x04


//
// AAL Parameters
//
typedef struct _AAL1_PARAMETERS
{
	UCHAR						Subtype;
	UCHAR						CBRRate;
	USHORT						Multiplier;							// optional (exception: zero if absent)
	UCHAR						SourceClockRecoveryMethod;			// optional
	UCHAR						ErrorCorrectionMethod;				// optional
	USHORT						StructuredDataTransferBlocksize;	// optional (exception: zero if absent)
	UCHAR						PartiallyFilledCellsMethod;			// optional
} AAL1_PARAMETERS, *PAAL1_PARAMETERS;

typedef struct _AAL34_PARAMETERS
{
	USHORT						ForwardMaxCPCSSDUSize;
	USHORT						BackwardMaxCPCSSDUSize;
	USHORT						LowestMID;					// optional
	USHORT						HighestMID;					// optional
	UCHAR						SSCSType;					// optional
} AAL34_PARAMETERS, *PAAL34_PARAMETERS;

typedef struct _AAL5_PARAMETERS
{
	ULONG						ForwardMaxCPCSSDUSize;
	ULONG						BackwardMaxCPCSSDUSize;
	UCHAR						Mode;
	UCHAR						SSCSType;					// optional
} AAL5_PARAMETERS, *PAAL5_PARAMETERS;

typedef struct _AALUSER_PARAMETERS
{
	ULONG						UserDefined;
} AALUSER_PARAMETERS, *PAALUSER_PARAMETERS;

typedef struct _AAL_PARAMETERS_IE
{
	ATM_AAL_TYPE				AALType;
	union
	{
		AAL1_PARAMETERS			AAL1Parameters;
		AAL34_PARAMETERS		AAL34Parameters;
		AAL5_PARAMETERS			AAL5Parameters;
		AALUSER_PARAMETERS		AALUserParameters;
	} AALSpecificParameters;

} AAL_PARAMETERS_IE, *PAAL_PARAMETERS_IE;

//
// ATM Traffic Descriptor
//
typedef struct _ATM_TRAFFIC_DESCRIPTOR	// For one direction
{
	ULONG						PeakCellRateCLP0;			// optional
	ULONG						PeakCellRateCLP01;			// optional
	ULONG						SustainableCellRateCLP0;	// optional
	ULONG						SustainableCellRateCLP01;	// optional
	ULONG						MaximumBurstSizeCLP0;		// optional
	ULONG						MaximumBurstSizeCLP01;		// optional
	BOOLEAN						Tagging;
} ATM_TRAFFIC_DESCRIPTOR, *PATM_TRAFFIC_DESCRIPTOR;


typedef struct _ATM_TRAFFIC_DESCRIPTOR_IE
{
	ATM_TRAFFIC_DESCRIPTOR		ForwardTD;
	ATM_TRAFFIC_DESCRIPTOR		BackwardTD;
	BOOLEAN						BestEffort;
} ATM_TRAFFIC_DESCRIPTOR_IE, *PATM_TRAFFIC_DESCRIPTOR_IE;


#ifdef MS_UNI4
//
// ATM Traffic Descriptor Add-On for UNI 4.0+
//
// REQUIREMENT: An add-on IE must follow immediately after its ancestor IE.
//
typedef struct _ATM_TRAFFIC_DESCRIPTOR_ADDON	// For one direction
{
	ULONG						ABRMinimumCellRateCLP01;	// optional
	BOOLEAN						FrameDiscard;
} ATM_TRAFFIC_DESCRIPTOR_ADDON, *PATM_TRAFFIC_DESCRIPTOR_ADDON;


typedef struct _ATM_TRAFFIC_DESCRIPTOR_IE_ADDON
{
	ATM_TRAFFIC_DESCRIPTOR_ADDON		ForwardTD;
	ATM_TRAFFIC_DESCRIPTOR_ADDON		BackwardTD;
} ATM_TRAFFIC_DESCRIPTOR_IE_ADDON, *PATM_TRAFFIC_DESCRIPTOR_IE_ADDON;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// Alternative Traffic Descriptor IE for UNI 4.0+
//
typedef struct ATM_ALTERNATIVE_TRAFFIC_DESCRIPTOR_IE
{
	ATM_TRAFFIC_DESCRIPTOR_IE			Part1;
	ATM_TRAFFIC_DESCRIPTOR_IE_ADDON		Part2;
} ATM_ALTERNATIVE_TRAFFIC_DESCRIPTOR_IE, *PATM_ALTERNATIVE_TRAFFIC_DESCRIPTOR_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// ATM Minimum Acceptable Traffic Descriptor
//
typedef struct _ATM_MINIMUM_TRAFFIC_DESCRIPTOR	// For one direction
{
	ULONG						PeakCellRateCLP0;			// optional
	ULONG						PeakCellRateCLP01;			// optional
	ULONG						MinimumCellRateCLP01;		// optional
} ATM_MINIMUM_TRAFFIC_DESCRIPTOR, *PATM_MINIMUM_TRAFFIC_DESCRIPTOR;


typedef struct _ATM_MINIMUM_TRAFFIC_DESCRIPTOR_IE
{
	ATM_MINIMUM_TRAFFIC_DESCRIPTOR		ForwardTD;
	ATM_MINIMUM_TRAFFIC_DESCRIPTOR		BackwardTD;
} ATM_MINIMUM_TRAFFIC_DESCRIPTOR_IE, *PATM_MINIMUM_TRAFFIC_DESCRIPTOR_IE;

#endif // MS_UNI4


//
// values used for the BearerClass field in the Broadband Bearer Capability
// and UNI 4.0 Broadband Bearer Capability IEs.
//


#define BCOB_A					0x00	// Bearer class A
#define BCOB_C					0x01	// Bearer class C
#define BCOB_X					0x02	// Bearer class X
#ifdef MS_UNI4_NOT_USED
#define BCOB_VP_SERVICE			0x03	// VP service
#endif

//
// values used for the TrafficType field in the Broadband Bearer Capability IE
//
#define TT_NOIND				0x00	// No indication of traffic type
#define TT_CBR					0x04	// Constant bit rate
#ifdef MS_UNI3_ERROR_KEPT
#define TT_VBR					0x06	// Variable bit rate
#else
#define TT_VBR					0x08	// Variable bit rate
#endif

//
// values used for the TimingRequirements field in the Broadband Bearer Capability IE
//
#define TR_NOIND				0x00	// No timing requirement indication
#define TR_END_TO_END			0x01	// End-to-end timing required
#define TR_NO_END_TO_END		0x02	// End-to-end timing not required

//
// values used for the ClippingSusceptability field in the Broadband Bearer Capability
// and UNI 4.0 Broadband Bearer Capability IEs.
//
#define CLIP_NOT				0x00	// Not susceptible to clipping
#define CLIP_SUS				0x20	// Susceptible to clipping

//
// values used for the UserPlaneConnectionConfig field in
// the Broadband Bearer Capability
// and UNI 4.0 Broadband Bearer Capability IEs.
//
#define UP_P2P					0x00	// Point-to-point connection
#define UP_P2MP					0x01	// Point-to-multipoint connection


//
// Broadband Bearer Capability for UNI 3.1
//
typedef struct _ATM_BROADBAND_BEARER_CAPABILITY_IE
{
	UCHAR			BearerClass;
	UCHAR			TrafficType;
	UCHAR			TimingRequirements;
	UCHAR			ClippingSusceptability;
	UCHAR			UserPlaneConnectionConfig;
} ATM_BROADBAND_BEARER_CAPABILITY_IE, *PATM_BROADBAND_BEARER_CAPABILITY_IE;


#ifdef MS_UNI4
//
// values used for the TransferCapability field in the UNI 4.0 Broadband Bearer Capability IE
//
									// Usage		Usage	Corresponding UNI3.x values
									// UNI 4.0		UNI 3.0	TrafficType	TimingRequirements
									// ------------	-------	-----------	------------------
#define XCAP_NRT_VBR_RCV_0	0x00	// Only rcv				NOIND		NOIND	   
#define XCAP_RT_VBR_RCV_1	0x01	// Only rcv				NOIND		END_TO_END
#define XCAP_NRT_VBR_RCV_2	0x02	// Only rcv				NOIND		NO_END_TO_END
#define XCAP_CBR_RCV_4		0x04	// Only rcv				CBR			NOIND	   
#define XCAP_CBR			0x05	// 						CBR			END_TO_END
#define XCAP_CBR_RCV_6		0x06	// Only rcv				CBR			NO_END_TO_END
#define XCAP_CBR_W_CLR		0x07	// 				None 	CBR			<reserved>
#define XCAP_NRT_VBR_RCV_8	0x08	// Only rcv				VBR			NOIND	   
#define XCAP_RT_VBR			0x09	// 						VBR			END_TO_END
#define XCAP_NRT_VBR		0x0A	// 						VBR			NO_END_TO_END
#define XCAP_NRT_VBR_W_CLR	0x0B	// 				None 	VBR			<reserved>
#define XCAP_ABR			0x0C	// 				None	<reserved>	NOIND	   
#define XCAP_RT_VBR_W_CLR	0x13	// 				None	(NOIND)	<reserved>

//
// Broadband Bearer Capability for UNI 4.0+
// Note: This can be used for UNI 3.1 as well.
//
typedef struct _ATM_BROADBAND_BEARER_CAPABILITY_IE_UNI40
{
	UCHAR			BearerClass;
	UCHAR			TransferCapability;				// optional
	UCHAR			ClippingSusceptability;
	UCHAR			UserPlaneConnectionConfig;
} ATM_BROADBAND_BEARER_CAPABILITY_IE_UNI40, *PATM_BROADBAND_BEARER_CAPABILITY_IE_UNI40;
#endif // MS_UNI4


//
// values used for the HighLayerInfoType field in ATM_BHLI
//
#define BHLI_ISO				0x00	// ISO
#define BHLI_UserSpecific		0x01	// User Specific
#define BHLI_HighLayerProfile	0x02	// High layer profile (only in UNI3.0)
#define BHLI_VendorSpecificAppId 0x03	// Vendor-Specific Application ID

//
// Broadband High layer Information
//
typedef struct _ATM_BHLI_IE
{
	ULONG			HighLayerInfoType;		// High Layer Information Type
	ULONG			HighLayerInfoLength;	// number of bytes in HighLayerInfo
	UCHAR			HighLayerInfo[8];		// The value dependent on the
											// HighLayerInfoType field
} ATM_BHLI_IE, *PATM_BHLI_IE;

//
// values used for Layer2Protocol in B-LLI
//
#define BLLI_L2_ISO_1745		0x01	// Basic mode ISO 1745
#define BLLI_L2_Q921			0x02	// CCITT Rec. Q.921
#define BLLI_L2_X25L			0x06	// CCITT Rec. X.25, link layer
#define BLLI_L2_X25M			0x07	// CCITT Rec. X.25, multilink
#define BLLI_L2_ELAPB			0x08	// Extended LAPB; for half duplex operation
#ifdef MS_UNI3_ERROR_KEPT
#define BLLI_L2_HDLC_NRM		0x09	// HDLC NRM (ISO 4335)
#define BLLI_L2_HDLC_ABM		0x0A	// HDLC ABM (ISO 4335)
#define BLLI_L2_HDLC_ARM		0x0B	// HDLC ARM (ISO 4335)
#else
#define BLLI_L2_HDLC_ARM		0x09	// HDLC ARM (ISO 4335)
#define BLLI_L2_HDLC_NRM		0x0A	// HDLC NRM (ISO 4335)
#define BLLI_L2_HDLC_ABM		0x0B	// HDLC ABM (ISO 4335)
#endif
#define BLLI_L2_LLC				0x0C	// LAN logical link control (ISO 8802/2)
#define BLLI_L2_X75				0x0D	// CCITT Rec. X.75, single link procedure
#define BLLI_L2_Q922			0x0E	// CCITT Rec. Q.922
#define BLLI_L2_USER_SPECIFIED	0x10	// User Specified
#define BLLI_L2_ISO_7776		0x11	// ISO 7776 DTE-DTE operation

//
// values used for Layer3Protocol in B-LLI
//
#define BLLI_L3_X25				0x06	// CCITT Rec. X.25, packet layer
#define BLLI_L3_ISO_8208		0x07	// ISO/IEC 8208 (X.25 packet layer for DTE
#define BLLI_L3_X223			0x08	// X.223/ISO 8878
#define BLLI_L3_SIO_8473		0x09	// ISO/IEC 8473 (OSI connectionless)
#define BLLI_L3_T70				0x0A	// CCITT Rec. T.70 min. network layer
#define BLLI_L3_ISO_TR9577		0x0B	// ISO/IEC TR 9577 Network Layer Protocol ID
#define BLLI_L3_USER_SPECIFIED	0x10	// User Specified
#ifdef MS_UNI4
#define BLLI_L3_H310			0x0C	// ITU H.310
#define BLLI_L3_H321			0x0D	// ITU H.321
#endif // MS_UNI4

//
// values used for Layer3IPI in struct B-LLI
//
#define BLLI_L3_IPI_SNAP		0x80	// IEEE 802.1 SNAP identifier
#define BLLI_L3_IPI_IP			0xCC	// Internet Protocol (IP) identifier

//
// Broadband Lower Layer Information
//
typedef struct _ATM_BLLI_IE
{
	ULONG						Layer2Protocol;
	UCHAR						Layer2Mode;
	UCHAR						Layer2WindowSize;
	ULONG						Layer2UserSpecifiedProtocol;
	ULONG						Layer3Protocol;
	UCHAR						Layer3Mode;
	UCHAR						Layer3DefaultPacketSize;
	UCHAR						Layer3PacketWindowSize;
	ULONG						Layer3UserSpecifiedProtocol;
	ULONG						Layer3IPI;
	UCHAR						SnapId[5];
} ATM_BLLI_IE, *PATM_BLLI_IE;


#ifdef MS_UNI4

//
// Values used for Layer3H310TerminalType in struct B-LLI ADDON
//
#define BLLI_L3_H310_TT_RECEIVE_ONLY		0x01
#define BLLI_L3_H310_TT_SEND_ONLY			0x02
#define BLLI_L3_H310_TT_RECEIVE_AND_SEND	0x03


//
// Values used for Layer3H310ForwardMultiplexingCapability
// and Layer3H310BackwardMultiplexingCapability in struct B-LLI ADDON
//
#define BLLI_L3_H310_MUX_NONE						0x00
#define BLLI_L3_H310_MUX_TRANSPORT_STREAM			0x01
#define BLLI_L3_H310_MUX_TRANSPORT_STREAM_WITH_FEC	0x02
#define BLLI_L3_H310_MUX_PROGRAM_STREAM				0x03
#define BLLI_L3_H310_MUX_PROGRAM_STREAM_WITH_FEC	0x04
#define BLLI_L3_H310_MUX_H221						0x05


//
// Broadband Lower Layer Information Add-On for UNI4.0+
//
// REQUIREMENT: An add-on IE must follow immediately after its ancestor IE.
//
typedef struct _ATM_BLLI_IE_ADDON
{
	UCHAR						Layer3H310TerminalType;						// optional
	UCHAR						Layer3H310ForwardMultiplexingCapability;	// optional
	UCHAR						Layer3H310BackwardMultiplexingCapability;	// optional
} ATM_BLLI_IE_ADDON, *PATM_BLLI_IE_ADDON;

#endif // MS_UNI4


//
// Values used for PresentationIndication in struct ATM_CALLING_PARTY_NUMBER_IE
//

#define CALLING_NUMBER_PRESENTATION_ALLOWED			0x00
#define CALLING_NUMBER_PRESENTATION_RESTRICTED		0x01
#define CALLING_NUMBER_PRESENTATION_NOT_AVAIL		0x02
#define CALLING_NUMBER_PRESENTATION_RESERVED		0x03

//
// Values used for ScreeningIndicator in struct ATM_CALLING_PARTY_NUMBER_IE
//

#define CALLING_NUMBER_SCREENING_USER_PROVIDED_NOT_SCREENED		0x00
#define CALLING_NUMBER_SCREENING_USER_PROVIDED_PASSED_SCREENING	0x01
#define CALLING_NUMBER_SCREENING_USER_PROVIDED_FAILED_SCREENING	0x02
#define CALLING_NUMBER_SCREENING_NW_PROVIDED		   			0x03


//
// Called Party Number
//
// If present, this IE overrides the Called Address specified in
// the main parameter block.
//
typedef ATM_ADDRESS	ATM_CALLED_PARTY_NUMBER_IE;


//
// Called Party Subaddress
//
typedef ATM_ADDRESS	ATM_CALLED_PARTY_SUBADDRESS_IE;



//
// Calling Party Number
//
typedef struct _ATM_CALLING_PARTY_NUMBER_IE
{
	ATM_ADDRESS					Number;
	UCHAR						PresentationIndication;		// optional
	UCHAR						ScreeningIndicator;			// optional
} ATM_CALLING_PARTY_NUMBER_IE, *PATM_CALLING_PARTY_NUMBER_IE;


//
// Calling Party Subaddress
//
typedef ATM_ADDRESS	ATM_CALLING_PARTY_SUBADDRESS_IE;


#ifdef MS_UNI4
//
// Connected Number IE for UNI 4.0 (for COLP Supplementary Services option)
//
typedef ATM_CALLING_PARTY_NUMBER_IE ATM_CONNECTED_NUMBER_IE, *PATM_CONNECTED_NUMBER_IE;


//
// Connected Subaddress IE for UNI 4.0 (for COLP Supplementary Services option)
//
typedef ATM_CALLING_PARTY_SUBADDRESS_IE ATM_CONNECTED_SUBADDRESS_IE, *PATM_CONNECTED_SUBADDRESS_IE;

#endif // MS_UNI4


//
// Values used for the QOSClassForward and QOSClassBackward
// fields in ATM_QOS_CLASS_IE
//
#define QOS_CLASS0				0x00
#define QOS_CLASS1				0x01
#define QOS_CLASS2				0x02
#define QOS_CLASS3				0x03
#define QOS_CLASS4				0x04

//
// QOS Class
//
typedef struct _ATM_QOS_CLASS_IE
{
	UCHAR						QOSClassForward;
	UCHAR						QOSClassBackward;
} ATM_QOS_CLASS_IE, *PATM_QOS_CLASS_IE;


#ifdef MS_UNI4
//
// Values used for the Origin field in ATM_EXTENDED_QOS_PARAMETERS_IE
//
#define ATM_XQOS_ORIGINATING_USER	0x00
#define ATM_XQOS_INTERMEDIATE_NW	0x01

//
// Extended QoS Parameters for UNI 4.0+
//
typedef struct _ATM_EXTENDED_QOS_PARAMETERS_IE
{
	UCHAR						Origin;
	UCHAR						Filler[3];
	ULONG						AcceptableForwardPeakCDV;	// optional
	ULONG						AcceptableBackwardPeakCDV;	// optional
	ULONG						CumulativeForwardPeakCDV;	// optional
	ULONG						CumulativeBackwardPeakCDV;	// optional
	UCHAR						AcceptableForwardCLR;		// optional
	UCHAR						AcceptableBackwardCLR;		// optional
} ATM_EXTENDED_QOS_PARAMETERS_IE, *PATM_EXTENDED_QOS_PARAMETERS_IE;

#endif // MS_UNI4


//
// Broadband Sending Complete
//
typedef struct _ATM_BROADBAND_SENDING_COMPLETE_IE
{
	UCHAR						SendingComplete;
} ATM_BROADBAND_SENDING_COMPLETE_IE, *PATM_BROADBAND_SENDING_COMPLETE_IE;


//
// Values used for the TypeOfNetworkId field in ATM_TRANSIT_NETWORK_SELECTION_IE
//
#define TNS_TYPE_NATIONAL			0x40

//
// Values used for the NetworkIdPlan field in ATM_TRANSIT_NETWORK_SELECTION_IE
//
#define TNS_PLAN_CARRIER_ID_CODE	0x01

//
// Transit Network Selection
//
typedef struct _ATM_TRANSIT_NETWORK_SELECTION_IE
{
	UCHAR						TypeOfNetworkId;
	UCHAR						NetworkIdPlan;
	UCHAR						NetworkIdLength;
	UCHAR						NetworkId[1];
} ATM_TRANSIT_NETWORK_SELECTION_IE, *PATM_TRANSIT_NETWORK_SELECTION_IE;


// 
// Values used for the Location field in struct ATM_CAUSE_IE
//
#define ATM_CAUSE_LOC_USER							0x00
#define ATM_CAUSE_LOC_PRIVATE_LOCAL					0x01
#define ATM_CAUSE_LOC_PUBLIC_LOCAL					0x02
#define ATM_CAUSE_LOC_TRANSIT_NETWORK				0x03
#define ATM_CAUSE_LOC_PUBLIC_REMOTE					0x04
#define ATM_CAUSE_LOC_PRIVATE_REMOTE				0x05
#ifdef MS_UNI3_ERROR_KEPT
#define ATM_CAUSE_LOC_INTERNATIONAL_NETWORK			0x06
#else
#define ATM_CAUSE_LOC_INTERNATIONAL_NETWORK			0x07
#endif
#define ATM_CAUSE_LOC_BEYOND_INTERWORKING			0x0A

// 
// Values used for the Cause field in struct ATM_CAUSE_IE
//
#ifdef MS_UNI4
#define ATM_CAUSE_UNALLOCATED_NUMBER				0x01
#define ATM_CAUSE_NO_ROUTE_TO_TRANSIT_NETWORK		0x02
#define ATM_CAUSE_NO_ROUTE_TO_DESTINATION			0x03
#define ATM_CAUSE_SEND_SPECIAL_TONE					0x04	// UNI 4.0+
#define ATM_CAUSE_MISDIALLED_TRUNK_PREFIX			0x05	// UNI 4.0+
#define ATM_CAUSE_CHANNEL_UNACCEPTABLE				0x06	// UNI 4.0+
#define ATM_CAUSE_CALL_AWARDED_IN_EST_CHAN			0x07	// UNI 4.0+
#define ATM_CAUSE_PREEMPTION						0x08	// UNI 4.0+
#define ATM_CAUSE_PREEMPTION_CIRC_RES_REUSE			0x09	// UNI 4.0+
#define ATM_CAUSE_VPI_VCI_UNACCEPTABLE				0x0A	// UNI 3.0 only!
#define ATM_CAUSE_NORMAL_CALL_CLEARING				0x10	// UNI 3.1+
#define ATM_CAUSE_USER_BUSY							0x11
#define ATM_CAUSE_NO_USER_RESPONDING				0x12
#define ATM_CAUSE_NO_ANSWER_FROM_USER_ALERTED		0x13	// UNI 4.0+
#define ATM_CAUSE_SUBSCRIBER_ABSENT					0x14	// UNI 4.0+
#define ATM_CAUSE_CALL_REJECTED						0x15
#define ATM_CAUSE_NUMBER_CHANGED					0x16
#define ATM_CAUSE_USER_REJECTS_CLIR					0x17
#define ATM_CAUSE_NONSELECTED_USER_CLEARING			0x1A	// UNI 4.0+
#define ATM_CAUSE_DESTINATION_OUT_OF_ORDER			0x1B
#define ATM_CAUSE_INVALID_NUMBER_FORMAT				0x1C
#define ATM_CAUSE_FACILITY_REJECTED					0x1D	// UNI 4.0+
#define ATM_CAUSE_STATUS_ENQUIRY_RESPONSE			0x1E
#define ATM_CAUSE_NORMAL_UNSPECIFIED				0x1F
#define ATM_CAUSE_TOO_MANY_ADD_PARTY				0x20	// UNI 4.0+
#define ATM_CAUSE_NO_CIRCUIT_CHANNEL_AVAIL			0x22	// UNI 4.0+
#define ATM_CAUSE_VPI_VCI_UNAVAILABLE				0x23
#define ATM_CAUSE_VPCI_VCI_ASSIGN_FAIL				0x24	// UNI 3.1+
#define ATM_CAUSE_USER_CELL_RATE_UNAVAILABLE		0x25	// UNI 3.1+
#define ATM_CAUSE_NETWORK_OUT_OF_ORDER				0x26
#define ATM_CAUSE_PFM_CONNECTION_OUT_OF_ORDER		0x27	// UNI 4.0+
#define ATM_CAUSE_PFM_CONNNECTION_OPERATIONAL		0x28	// UNI 4.0+
#define ATM_CAUSE_TEMPORARY_FAILURE					0x29
#define ATM_CAUSE_SWITCH_EQUIPM_CONGESTED			0x2A	// UNI 4.0+
#define ATM_CAUSE_ACCESS_INFORMAION_DISCARDED		0x2B
#define ATM_CAUSE_REQUESTED_CIRC_CHANNEL_NOT_AVAIL	0x2C	// UNI 4.0+
#define ATM_CAUSE_NO_VPI_VCI_AVAILABLE				0x2D
#define ATM_CAUSE_RESOURCE_UNAVAILABLE				0x2F
#define ATM_CAUSE_QOS_UNAVAILABLE					0x31
#define ATM_CAUSE_REQ_FACILITY_NOT_SUBSCRIBED		0x32	// UNI 4.0+
#define ATM_CAUSE_USER_CELL_RATE_UNAVAILABLE__UNI30	0x33	// UNI 3.0 only!
#define ATM_CAUSE_OUTG_CALLS_BARRED_W_CUG			0x35	// UNI 4.0+
#define ATM_CAUSE_INCOM_CALLS_BARRED_W_CUG			0x37	// UNI 4.0+
#define ATM_CAUSE_BEARER_CAPABILITY_UNAUTHORIZED	0x39
#define ATM_CAUSE_BEARER_CAPABILITY_UNAVAILABLE		0x3A
#define ATM_CAUSE_INCONSIST_OUTG_ACCESS_INFO		0x3E	// UNI 4.0+
#define ATM_CAUSE_OPTION_UNAVAILABLE				0x3F
#define ATM_CAUSE_BEARER_CAPABILITY_UNIMPLEMENTED	0x41
#define ATM_CAUSE_CHANNEL_TYPE_NOT_IMPLEMENTED		0x42	// UNI 4.0+
#define ATM_CAUSE_REQ_FACILITY_NOT_IMPLEMENTED		0x45	// UNI 4.0+
#define ATM_CAUSE_ONLY_RESTR_BEAR_CAP_AVAIL			0x46	// UNI 4.0+
#define ATM_CAUSE_UNSUPPORTED_TRAFFIC_PARAMETERS	0x49
#define ATM_CAUSE_AAL_PARAMETERS_UNSUPPORTED		0x4E	// UNI 3.1+
#define ATM_CAUSE_SERVICE_OPTION_NOT_AVAIL			0x4F	// UNI 4.0+
#define ATM_CAUSE_INVALID_CALL_REFERENCE			0x51
#define ATM_CAUSE_CHANNEL_NONEXISTENT				0x52
#define ATM_CAUSE_SUSP_CALL_EXISTS_NOT_CALL_ID		0x53	// UNI 4.0+
#define ATM_CAUSE_CALL_ID_IN_USE					0x54	// UNI 4.0+
#define ATM_CAUSE_NO_CALL_SUSPENDED					0x55	// UNI 4.0+
#define ATM_CAUSE_CALL_W_REQ_ID_CLEARED				0x56	// UNI 4.0+
#define ATM_CAUSE_USER_NOT_MEMBER_CUG				0x57	// UNI 4.0+
#define ATM_CAUSE_INCOMPATIBLE_DESTINATION			0x58
#define ATM_CAUSE_INVALID_ENDPOINT_REFERENCE		0x59
#define ATM_CAUSE_NON_EXISTENT_CUG 					0x5A	// UNI 4.0+
#define ATM_CAUSE_INVALID_TRANSIT_NETWORK_SELECTION	0x5B
#define ATM_CAUSE_TOO_MANY_PENDING_ADD_PARTY		0x5C
#define ATM_CAUSE_AAL_PARAMETERS_UNSUPPORTED__UNI30	0x5D	// UNI 3.0 only!
#define ATM_CAUSE_INVALID_MSG_UNSPECIFIED			0x5F	// UNI 4.0+
#define ATM_CAUSE_MANDATORY_IE_MISSING				0x60
#define ATM_CAUSE_UNIMPLEMENTED_MESSAGE_TYPE		0x61
#define ATM_CAUSE_MSG_CONFL_STATE_OR_UNIMPL			0x62	// UNI 4.0+
#define ATM_CAUSE_UNIMPLEMENTED_IE					0x63
#define ATM_CAUSE_INVALID_IE_CONTENTS				0x64
#define ATM_CAUSE_INVALID_STATE_FOR_MESSAGE			0x65
#define ATM_CAUSE_RECOVERY_ON_TIMEOUT				0x66
#define ATM_CAUSE_IE_INVAL_UNIMPL_PASSED_ON			0x67	// UNI 4.0+
#define ATM_CAUSE_INCORRECT_MESSAGE_LENGTH			0x68
#define ATM_CAUSE_UNRECOGNIZED_PARM_MSG_DISCARDED	0x6E	// UNI 4.0+
#define ATM_CAUSE_PROTOCOL_ERROR					0x6F
#define ATM_CAUSE_INTERWORKING_UNSPECIFIED			0x7F	// UNI 4.0+

#else // !MS_UNI4

#define ATM_CAUSE_UNALLOCATED_NUMBER				0x01
#define ATM_CAUSE_NO_ROUTE_TO_TRANSIT_NETWORK		0x02
#define ATM_CAUSE_NO_ROUTE_TO_DESTINATION			0x03
#define ATM_CAUSE_VPI_VCI_UNACCEPTABLE				0x0A
#define ATM_CAUSE_NORMAL_CALL_CLEARING				0x10
#define ATM_CAUSE_USER_BUSY							0x11
#define ATM_CAUSE_NO_USER_RESPONDING				0x12
#define ATM_CAUSE_CALL_REJECTED						0x15
#define ATM_CAUSE_NUMBER_CHANGED					0x16
#define ATM_CAUSE_USER_REJECTS_CLIR					0x17
#define ATM_CAUSE_DESTINATION_OUT_OF_ORDER			0x1B
#define ATM_CAUSE_INVALID_NUMBER_FORMAT				0x1C
#define ATM_CAUSE_STATUS_ENQUIRY_RESPONSE			0x1E
#define ATM_CAUSE_NORMAL_UNSPECIFIED				0x1F
#define ATM_CAUSE_VPI_VCI_UNAVAILABLE				0x23
#define ATM_CAUSE_NETWORK_OUT_OF_ORDER				0x26
#define ATM_CAUSE_TEMPORARY_FAILURE					0x29
#define ATM_CAUSE_ACCESS_INFORMAION_DISCARDED		0x2B
#define ATM_CAUSE_NO_VPI_VCI_AVAILABLE				0x2D
#define ATM_CAUSE_RESOURCE_UNAVAILABLE				0x2F
#define ATM_CAUSE_QOS_UNAVAILABLE					0x31
#define ATM_CAUSE_USER_CELL_RATE_UNAVAILABLE		0x33
#define ATM_CAUSE_BEARER_CAPABILITY_UNAUTHORIZED	0x39
#define ATM_CAUSE_BEARER_CAPABILITY_UNAVAILABLE		0x3A
#define ATM_CAUSE_OPTION_UNAVAILABLE				0x3F
#define ATM_CAUSE_BEARER_CAPABILITY_UNIMPLEMENTED	0x41
#define ATM_CAUSE_UNSUPPORTED_TRAFFIC_PARAMETERS	0x49
#define ATM_CAUSE_INVALID_CALL_REFERENCE			0x51
#define ATM_CAUSE_CHANNEL_NONEXISTENT				0x52
#define ATM_CAUSE_INCOMPATIBLE_DESTINATION			0x58
#define ATM_CAUSE_INVALID_ENDPOINT_REFERENCE		0x59
#define ATM_CAUSE_INVALID_TRANSIT_NETWORK_SELECTION	0x5B
#define ATM_CAUSE_TOO_MANY_PENDING_ADD_PARTY		0x5C
#define ATM_CAUSE_AAL_PARAMETERS_UNSUPPORTED		0x5D
#define ATM_CAUSE_MANDATORY_IE_MISSING				0x60
#define ATM_CAUSE_UNIMPLEMENTED_MESSAGE_TYPE		0x61
#define ATM_CAUSE_UNIMPLEMENTED_IE					0x63
#define ATM_CAUSE_INVALID_IE_CONTENTS				0x64
#define ATM_CAUSE_INVALID_STATE_FOR_MESSAGE			0x65
#define ATM_CAUSE_RECOVERY_ON_TIMEOUT				0x66
#define ATM_CAUSE_INCORRECT_MESSAGE_LENGTH			0x68
#define ATM_CAUSE_PROTOCOL_ERROR					0x6F
#endif // !MS_UNI4

//
// Values used for the Condition portion of the Diagnostics field
// in struct ATM_CAUSE_IE, for certain Cause values
//
#define ATM_CAUSE_COND_UNKNOWN						0x00
#define ATM_CAUSE_COND_PERMANENT					0x01
#define ATM_CAUSE_COND_TRANSIENT					0x02

//
// Values used for the Rejection Reason portion of the Diagnostics field
// in struct ATM_CAUSE_IE, for certain Cause values
//
#define ATM_CAUSE_REASON_USER						0x00
#define ATM_CAUSE_REASON_IE_MISSING					0x04
#define ATM_CAUSE_REASON_IE_INSUFFICIENT			0x08

//
// Values used for the P-U flag of the Diagnostics field
// in struct ATM_CAUSE_IE, for certain Cause values
//
#define ATM_CAUSE_PU_PROVIDER						0x00
#define ATM_CAUSE_PU_USER							0x08

//
// Values used for the N-A flag of the Diagnostics field
// in struct ATM_CAUSE_IE, for certain Cause values
//
#define ATM_CAUSE_NA_NORMAL							0x00
#define ATM_CAUSE_NA_ABNORMAL						0x04

#ifdef MS_UNI4
//
// values used for placing IE identifiers in the Diagnostics field
//
#define	ATM_CAUSE_DIAG_IE_NARROW_BEARER_CAPABILITY	0x04
#define	ATM_CAUSE_DIAG_IE_CAUSE						0x08
#define	ATM_CAUSE_DIAG_IE_CALL_STATE				0x14
#define	ATM_CAUSE_DIAG_IE_PROGRESS_IND				0x1E
#define	ATM_CAUSE_DIAG_IE_NOTIF_IND					0x27
#define	ATM_CAUSE_DIAG_IE_END_TO_END_TDELAY			0x42
#define	ATM_CAUSE_DIAG_IE_CONNECTED_NUMBER			0x4C
#define	ATM_CAUSE_DIAG_IE_CONNECTED_SUBADDR			0x4D
#define	ATM_CAUSE_DIAG_IE_ENDPOINT_REF				0x54
#define	ATM_CAUSE_DIAG_IE_ENDPOINT_STATE			0x55
#define	ATM_CAUSE_DIAG_IE_AAL_PARMS					0x58
#define	ATM_CAUSE_DIAG_IE_TRAFFIC_DESCRIPTOR		0x59
#define	ATM_CAUSE_DIAG_IE_CONNECTION_ID				0x5A
#define ATM_CAUSE_DIAG_IE_OAM_TRAFFIC_DESCRIPTOR	0x5B
#define	ATM_CAUSE_DIAG_IE_QOS						0x5C
#define	ATM_CAUSE_DIAG_IE_HIGH_LAYER_INFO			0x5D
#define	ATM_CAUSE_DIAG_IE_BEARER_CAPABILITY			0x5E
#define	ATM_CAUSE_DIAG_IE_LOW_LAYER_INFO			0x5F
#define	ATM_CAUSE_DIAG_IE_LOCKING_SHIFT				0x60
#define	ATM_CAUSE_DIAG_IE_NON_LOCKING_SHIFT			0x61
#define	ATM_CAUSE_DIAG_IE_SENDING_COMPLETE			0x62
#define	ATM_CAUSE_DIAG_IE_REPEAT_INDICATOR			0x63
#define	ATM_CAUSE_DIAG_IE_CALLING_PARTY_NUMBER		0x6C
#define	ATM_CAUSE_DIAG_IE_CALLING_PARTY_SUBADDR		0x6D
#define	ATM_CAUSE_DIAG_IE_CALLED_PARTY_NUMBER		0x70
#define	ATM_CAUSE_DIAG_IE_CALLED_PARTY_SUBADDR		0x71
#define	ATM_CAUSE_DIAG_IE_TRANSIT_NETWORK_SELECT	0x78
#define	ATM_CAUSE_DIAG_IE_RESTART_INDICATOR			0x79
#define	ATM_CAUSE_DIAG_IE_NARROW_LOW_LAYER_COMPAT	0x7C
#define	ATM_CAUSE_DIAG_IE_NARROW_HIGH_LAYER_COMPAT	0x7D
#define	ATM_CAUSE_DIAG_IE_USER_USER					0x7E
#define	ATM_CAUSE_DIAG_IE_GENERIC_ID				0x7F
#define	ATM_CAUSE_DIAG_IE_MIN_TRAFFIC_DESCRIPTOR	0x81
#define	ATM_CAUSE_DIAG_IE_ALT_TRAFFIC_DESCRIPTOR	0x82
#define	ATM_CAUSE_DIAG_IE_ABR_SETUP_PARMS			0x84
#define	ATM_CAUSE_DIAG_IE_CALLED_SOFT_PVPC_PVCC		0xE0
#define	ATM_CAUSE_DIAG_IE_CRANKBACK					0xE1
#define	ATM_CAUSE_DIAG_IE_DESIGNATED_TRANSIT_LIST	0xE2
#define	ATM_CAUSE_DIAG_IE_CALLING_SOFT_PVPC_PVCC	0xE3
#define	ATM_CAUSE_DIAG_IE_ABR_ADD_PARMS				0xE4
#define	ATM_CAUSE_DIAG_IE_LIJ_CALL_ID				0xE8
#define	ATM_CAUSE_DIAG_IE_LIJ_PARMS					0xE9
#define	ATM_CAUSE_DIAG_IE_LEAF_SEQ_NO				0xEA
#define	ATM_CAUSE_DIAG_IE_CONNECTION_SCOPE_SELECT	0xEB
#define	ATM_CAUSE_DIAG_IE_EXTENDED_QOS				0xEC

//
// values used for placing IE subfield identifiers in the Diagnostics field
//
#define ATM_CAUSE_DIAG_RATE_ID_FW_PEAK_CLP0			0x82
#define ATM_CAUSE_DIAG_RATE_ID_BW_PEAK_CLP0			0x83
#define ATM_CAUSE_DIAG_RATE_ID_FW_PEAK_CLP01		0x84
#define ATM_CAUSE_DIAG_RATE_ID_BW_PEAK_CLP01		0x85
#define ATM_CAUSE_DIAG_RATE_ID_FW_SUST_CLP0			0x88
#define ATM_CAUSE_DIAG_RATE_ID_BW_SUST_CLP0			0x89
#define ATM_CAUSE_DIAG_RATE_ID_FW_SUST_CLP01		0x90
#define ATM_CAUSE_DIAG_RATE_ID_BW_SUST_CLP01		0x91
#define ATM_CAUSE_DIAG_RATE_ID_FW_ABR_MIN_CLP01		0x92
#define ATM_CAUSE_DIAG_RATE_ID_BW_ABR_MIN_CLP01		0x93
#define ATM_CAUSE_DIAG_RATE_ID_FW_BURST_CLP0		0xA0
#define ATM_CAUSE_DIAG_RATE_ID_BW_BURST_CLP0		0xA1
#define ATM_CAUSE_DIAG_RATE_ID_FW_BURST_CLP01		0xB0
#define ATM_CAUSE_DIAG_RATE_ID_BW_BURST_CLP01		0xB1
#define ATM_CAUSE_DIAG_RATE_ID_BEST_EFFORT			0xBE
#define ATM_CAUSE_DIAG_RATE_ID_TM_OPTIONS			0xBF

//
// Values used for placing a CCBS indicator in the Diagnostics field 
//
#define ATM_CAUSE_DIAG_CCBS_SPARE					0x00
#define ATM_CAUSE_DIAG_CCBS_CCBS_POSSIBLE			0x01
#define ATM_CAUSE_DIAG_CCBS_CCBS_NOT_POSSIBLE		0x02

//
// Values used for placing Attribute Numbers in the Diagnostics field
//
#define ATM_CAUSE_DIAG_ATTR_NO_INFO_XFER_CAP		0x31
#define ATM_CAUSE_DIAG_ATTR_NO_INFO_XFER_MODE		0x32
#define ATM_CAUSE_DIAG_ATTR_NO_INFO_XFER_RATE		0x33
#define ATM_CAUSE_DIAG_ATTR_NO_STRUCTURE			0x34
#define ATM_CAUSE_DIAG_ATTR_NO_CONFIG				0x35
#define ATM_CAUSE_DIAG_ATTR_NO_ESTABL				0x36
#define ATM_CAUSE_DIAG_ATTR_NO_SYMMETRY				0x37
#define ATM_CAUSE_DIAG_ATTR_NO_INFO_XFER_RATE2		0x38
#define ATM_CAUSE_DIAG_ATTR_NO_LAYER_ID				0x39
#define ATM_CAUSE_DIAG_ATTR_NO_RATE_MULT			0x3A

#endif // MS_UNI4


//
// Cause
//
// Note: If used as empty buffer for response IE, then the DiagnosticsLength
//		 MUST be filled in correctly, to show available buffer length.
//
typedef struct _ATM_CAUSE_IE
{
	UCHAR						Location;
	UCHAR						Cause;
	UCHAR						DiagnosticsLength;
	UCHAR						Diagnostics[4];		// Variable length information (minimum 4 bytes)
} ATM_CAUSE_IE, *PATM_CAUSE_IE;


#ifdef MS_UNI4
//
// Connection Identifier IE for UNI 4.0+
//
// Setting VPCI to SAP_FIELD_ABSENT, means it is "VP-associated" signalling,
// (which is not supported by this CallManager);
//
// Otherwise we assume "explicit VPCI" (this is the normal setting),
// and the Vpci value is used.
//
// Setting the VCI to SAP_FIELD_ABSENT, means it is "switched VP",
// which is not supported by this CallManager;
//
// Setting the VCI to SAP_FIELD_ANY, means it is "explicit VPCI, any VCI";
//
// Setting both the Vpci and the Vci fields to SAP_FIELD_ANY,
// or setting both the Vpci and the Vci fields to SAP_FIELD_ABSENT,
// means that the ConnectionId is not defined,
// so it will not be sent to the peer.
// (This allows an empty ConnectionIdentifier IE to hold the response).
//
// Otherwise we assume "explicit VPCI, explicit VCI" (this is the normal
// setting), and the Vci value is used.
//
typedef struct _ATM_CONNECTION_ID_IE
{
	ULONG						Vpci;	// Optional: Can use SAP_FIELD_ANY or SAP_FIELD_ABSENT here.
	ULONG						Vci;	// Optional: Can use SAP_FIELD_ANY or SAP_FIELD_ABSENT here.
} ATM_CONNECTION_ID_IE, *PATM_CONNECTION_ID_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// End-to-End Transit Delay IE for UNI 4.0+
//
typedef struct _ATM_END_TO_END_TRANSIT_DELAY_IE
{
	ULONG						CumulativeTransitDelay;			// optional (Milliseconds)
	ULONG						MaximumEndToEndTransitDelay;	// optional (Milliseconds)
	BOOLEAN						NetworkGenerated;
} ATM_END_TO_END_TRANSIT_DELAY_IE, *PATM_END_TO_END_TRANSIT_DELAY_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// Notification Indicator IE for UNI 4.0+
//
// Note: If used as empty buffer for response IE, then the InformationLength
//		 MUST be filled in correctly, to show available buffer length.
//
typedef struct _ATM_NOTIFICATION_INDICATOR_IE
{
	USHORT						NotificationId;
	USHORT						InformationLength;
	UCHAR						NotificationInformation[1];		// Variable length information
} ATM_NOTIFICATION_INDICATOR_IE, *PATM_NOTIFICATION_INDICATOR_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// User-User IE for UNI 4.0+ (for UUS Supplementary Services option)
//
// Note: If used as empty buffer for response IE, then the InformationLength
//		 MUST be filled in correctly, to show available buffer length.
//
typedef struct _ATM_USER_USER_IE
{
	UCHAR						ProtocolDescriminator;
	UCHAR						Filler[1];
	USHORT						InformationLength;
	UCHAR						UserUserInformation[1];			// Variable length information
} ATM_USER_USER_IE, *PATM_USER_USER_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// Generic ID Transport IE for UNI 4.0+
//
// Note: If used as empty buffer for response IE, then the InformationLength
//		 MUST be filled in correctly, to show available buffer length.
//
typedef struct _ATM_GENERIC_ID_TRANSPORT_IE
{
	USHORT						InformationLength;
	UCHAR						GenericIDInformation[1];		// Variable length information
} ATM_GENERIC_ID_TRANSPORT_IE, *PATM_GENERIC_ID_TRANSPORT_IE;

#endif // MS_UNI4


//
// Leaf Initiated Join (LIJ) Identifier
//
typedef struct _ATM_LIJ_CALLID_IE
{
	ULONG						Identifier;
} ATM_LIJ_CALLID_IE, *PATM_LIJ_CALLID_IE;


#ifdef MS_UNI4
//
// Values used for the ScreeningIndication field in struct ATM_LIJ_PARAMETERS_IE
//
#define ATM_LIJ_PARMS_SCREEN_NO_ROOT_NOTIF	0x00

//
// Leaf Initiated Join (LIJ) Parameters IE for UNI 4.0+
//
typedef struct _ATM_LIJ_PARAMETERS_IE
{
	UCHAR						ScreeningIndication;
} ATM_LIJ_PARAMETERS_IE, *PATM_LIJ_PARAMETERS_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// Leaf Sequence Number IE for UNI 4.0+
//
typedef struct _ATM_LEAF_SEQUENCE_NUMBER_IE
{
	ULONG						SequenceNumber;
} ATM_LEAF_SEQUENCE_NUMBER_IE, *PATM_LEAF_SEQUENCE_NUMBER_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// Values used for the ConnectionScopeType field in _ATM_CONNECTION_SCOPE_SELECTION_IE
//
#define ATM_SCOPE_TYPE_ORGANIZATIONAL				0x01

//
// Values used for the ConnectionScopeSelection field in _ATM_CONNECTION_SCOPE_SELECTION_IE
//
#define ATM_SCOPE_ORGANIZATIONAL_LOCAL_NW			0x01
#define ATM_SCOPE_ORGANIZATIONAL_LOCAL_NW_PLUS1		0x02
#define ATM_SCOPE_ORGANIZATIONAL_LOCAL_NW_PLUS2		0x03
#define ATM_SCOPE_ORGANIZATIONAL_SITE_MINUS1		0x04
#define ATM_SCOPE_ORGANIZATIONAL_INTRA_SITE			0x05
#define ATM_SCOPE_ORGANIZATIONAL_SITE_PLUS1			0x06
#define ATM_SCOPE_ORGANIZATIONAL_ORG_MINUS1			0x07
#define ATM_SCOPE_ORGANIZATIONAL_INTRA_ORG			0x08
#define ATM_SCOPE_ORGANIZATIONAL_ORG_PLUS1			0x09
#define ATM_SCOPE_ORGANIZATIONAL_COMM_MINUS1		0x0A
#define ATM_SCOPE_ORGANIZATIONAL_INTRA_COMM			0x0B
#define ATM_SCOPE_ORGANIZATIONAL_COMM_PLUS1			0x0C
#define ATM_SCOPE_ORGANIZATIONAL_REGIONAL			0x0D
#define ATM_SCOPE_ORGANIZATIONAL_INTER_REGIONAL		0x0E
#define ATM_SCOPE_ORGANIZATIONAL_GLOBAL				0x0F

//
// Connection Scope Selection IE for UNI 4.0+
//
typedef struct _ATM_CONNECTION_SCOPE_SELECTION_IE
{
	UCHAR						ConnectionScopeType;
	UCHAR						ConnectionScopeSelection;
} ATM_CONNECTION_SCOPE_SELECTION_IE, *PATM_CONNECTION_SCOPE_SELECTION_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// Values used for the XxxxAdditionalParameters fields in _ATM_ABR_ADDITIONAL_PARAMETERS_IE
//
#define			ATM_ABR_PARMS_NRM_PRESENT		0x80000000
#define			ATM_ABR_PARMS_TRM_PRESENT		0x40000000
#define			ATM_ABR_PARMS_CDF_PRESENT		0x20000000
#define			ATM_ABR_PARMS_ADTF_PRESENT		0x10000000
#define			ATM_ABR_PARMS_NRM_MASK			0x0E000000
#define			ATM_ABR_PARMS_NRM_SHIFT			25
#define			ATM_ABR_PARMS_TRM_MASK			0x01C00000
#define			ATM_ABR_PARMS_TRM_SHIFT			22
#define			ATM_ABR_PARMS_CDF_MASK			0x00380000
#define			ATM_ABR_PARMS_CDF_SHIFT			19
#define			ATM_ABR_PARMS_ADTF_MASK			0x0007FE00
#define			ATM_ABR_PARMS_ADTF_SHIFT		9
#define 		ATM_ABR_PARMS_NRM_DEFAULT		4
#define 		ATM_ABR_PARMS_TRM_DEFAULT		7
#define 		ATM_ABR_PARMS_CDF_DEFAULT		3
#define 		ATM_ABR_PARMS_ADTF_DEFAULT		50

typedef struct _ATM_ABR_ADDITIONAL_PARAMETERS_IE
{
	ULONG	ForwardAdditionalParameters;
	ULONG	BackwardAdditionalParameters;
} ATM_ABR_ADDITIONAL_PARAMETERS_IE, *PATM_ABR_ADDITIONAL_PARAMETERS_IE;

#endif // MS_UNI4


#ifdef MS_UNI4
//
// ABR Setup Parameters IE for UNI 4.0+
//
typedef struct _ATM_ABR_SETUP_PARAMETERS_IE
{
	ULONG	ForwardABRInitialCellRateCLP01;			// optional
	ULONG	BackwardABRInitialCellRateCLP01;		// optional
	ULONG	ForwardABRTransientBufferExposure;		// optional
	ULONG	BackwardABRTransientBufferExposure;		// optional
	ULONG	CumulativeRmFixedRTT;
	UCHAR	ForwardABRRateIncreaseFactorLog2;		// optional (0..15 [log2(RIF*32768)])
	UCHAR	BackwardABRRateIncreaseFactorLog2;		// optional (0..15 [log2(RIF*32768)])
	UCHAR	ForwardABRRateDecreaseFactorLog2;		// optional (0..15 [log2(RDF*32768)])
	UCHAR	BackwardABRRateDecreaseFactorLog2;		// optional (0..15 [log2(RDF*32768)])
} ATM_ABR_SETUP_PARAMETERS_IE, *PATM_ABR_SETUP_PARAMETERS_IE;

#endif // MS_UNI4


//
// Raw Information Element - the user can fill in whatever he wants
//
typedef struct _ATM_RAW_IE
{
	ULONG						RawIELength;
	ULONG						RawIEType;
	UCHAR						RawIEValue[1];		// Vaiable length information
} ATM_RAW_IE, *PATM_RAW_IE;


//
// This is the value of the ParamType field in the CO_SPECIFIC_PARAMETERS structure
// when the Parameters[] field contains ATM media specific values in the structure
// ATM_MEDIA_PARAMETERS.
//
#define ATM_MEDIA_SPECIFIC		0x00000001

//
// The Q2931 Call Manager Specific parameters that goes into the
// CallMgrParameters->CallMgrSpecific.Parameters
//
typedef struct _Q2931_CALLMGR_PARAMETERS
{
	ATM_ADDRESS					CalledParty;
	ATM_ADDRESS					CallingParty;
	ULONG						InfoElementCount;
	UCHAR						InfoElements[1];	// one or more info elements
} Q2931_CALLMGR_PARAMETERS, *PQ2931_CALLMGR_PARAMETERS;


//
// This is the specific portion of either the Media parameters or the CallMgr
// Parameters. The following define is used in the ParamType field
// depending on the signaling type.
//
#define CALLMGR_SPECIFIC_Q2931	0x00000001

typedef struct _ATM_VPIVCI
{
	ULONG						Vpi;
	ULONG						Vci;
} ATM_VPIVCI, *PATM_VPIVCI;

//
// ATM Service Category
//
#define	ATM_SERVICE_CATEGORY_CBR	1	// Constant Bit Rate
#define	ATM_SERVICE_CATEGORY_VBR	2	// Variable Bit Rate
#define	ATM_SERVICE_CATEGORY_UBR	4	// Unspecified Bit Rate
#define	ATM_SERVICE_CATEGORY_ABR	8	// Available Bit Rate

typedef ULONG	ATM_SERVICE_CATEGORY, *PATM_SERVICE_CATEGORY;


//
// ATM flow parameters for use in specifying Media parameters
//
typedef struct _ATM_FLOW_PARAMETERS
{
	ATM_SERVICE_CATEGORY		ServiceCategory;
	ULONG						AverageCellRate;			// in cells/sec
	ULONG						PeakCellRate;				// in cells/sec
	ULONG						MinimumCellRate;			// in cells/sec (ABR MCR)
	ULONG						InitialCellRate;			// in cells/sec (ABR ICR)
	ULONG						BurstLengthCells;			// in cells
	ULONG						MaxSduSize;					// MTU in bytes
	ULONG						TransientBufferExposure;	// in cells (ABR TBE)
	ULONG						CumulativeRMFixedRTT;		// in microseconds (ABR FRTT)
	UCHAR						RateIncreaseFactor;			// UNI 4.0 coding (ABR RIF)
	UCHAR						RateDecreaseFactor;			// UNI 4.0 coding (ABR RDF)
	USHORT						ACRDecreaseTimeFactor;		// UNI 4.0 coding (ABR ADTF)
	UCHAR						MaximumCellsPerForwardRMCell; // UNI 4.0 coding (ABR Nrm)
	UCHAR						MaximumForwardRMCellInterval; // UNI 4.0 coding (ABR Trm)
	UCHAR						CutoffDecreaseFactor;		// UNI 4.0 coding (ABR CDF)
	UCHAR						Reserved1;					// padding
	ULONG						MissingRMCellCount;			// (ABR CRM)
	ULONG						Reserved2;
	ULONG						Reserved3;
} ATM_FLOW_PARAMETERS, *PATM_FLOW_PARAMETERS;

//
// ATM Specific Media parameters - this is the Media specific structure for ATM
// that goes into MediaParameters->MediaSpecific.Parameters.
//
typedef struct _ATM_MEDIA_PARAMETERS
{
	ATM_VPIVCI					ConnectionId;
	ATM_AAL_TYPE				AALType;
	ULONG						CellDelayVariationCLP0;
	ULONG						CellDelayVariationCLP1;
	ULONG						CellLossRatioCLP0;
	ULONG						CellLossRatioCLP1;
	ULONG						CellTransferDelayCLP0;
	ULONG						CellTransferDelayCLP1;
	ULONG						DefaultCLP;
	ATM_FLOW_PARAMETERS			Transmit;
	ATM_FLOW_PARAMETERS			Receive;
} ATM_MEDIA_PARAMETERS, *PATM_MEDIA_PARAMETERS;

#ifdef MS_UNI4
//
// Generic ATM Call Manager Specific parameters.
// These may be used for call/leaf teardown and for certain NdisCoRequest calls.
//
// When used for closing a call and for dropping a leaf, by either client or
// Call Manager, this struct is referenced by the `Buffer` parameter of
// the relevant NDIS functions.
//
// When used for NdisCoRequest calls to make ATM UNI protocol specific
// requests and indications
//		OID_ATM_CALL_PROCEEDING
//		OID_ATM_CALL_ALERTING
//		OID_ATM_LEAF_ALERTING
//		OID_ATM_CALL_NOTIFY
// this struct is referenced by the `InformationBuffer` parameter.
//
typedef struct _Q2931_CALLMGR_SUBSEQUENT_PARAMETERS
{
	ULONG						InfoElementCount;
	UCHAR						InfoElements[1];	// one or more info elements
} Q2931_CALLMGR_SUBSEQUENT_PARAMETERS, *PQ2931_CALLMGR_SUBSEQUENT_PARAMETERS;
#endif // MS_UNI4

#ifdef MS_UNI4
//
// Field values that may be used whereever applicable to signify absence
// of data or wildcard data.
//
#endif // MS_UNI4

#ifndef SAP_FIELD_ABSENT
#define SAP_FIELD_ABSENT		((ULONG)0xfffffffe)
#endif

#ifndef SAP_FIELD_ABSENT_USHORT
#define SAP_FIELD_ABSENT_USHORT	((USHORT)0xfffe)
#endif

#ifndef SAP_FIELD_ABSENT_UCHAR
#define SAP_FIELD_ABSENT_UCHAR	((UCHAR)0xfe)
#endif

#ifndef SAP_FIELD_ANY
#define SAP_FIELD_ANY			((ULONG)0xffffffff)
#endif

#define SAP_FIELD_ANY_AESA_SEL	((ULONG)0xfffffffa)	// SEL is wild-carded
#define SAP_FIELD_ANY_AESA_REST	((ULONG)0xfffffffb)	// All of the address
													// except SEL, is wild-carded

//
// The ATM Specific SAP definition
//
typedef struct _ATM_SAP
{
	ATM_BLLI_IE					Blli;
	ATM_BHLI_IE					Bhli;
	ULONG						NumberOfAddresses;
	UCHAR						Addresses[1];	// each of type ATM_ADDRESS
} ATM_SAP, *PATM_SAP;

//
// The ATM Specific SAP definition when adding PVCs
//
typedef struct _ATM_PVC_SAP
{
	ATM_BLLI_IE					Blli;
	ATM_BHLI_IE					Bhli;
} ATM_PVC_SAP, *PATM_PVC_SAP;

//
// The structure passed in the Parameters field of the CO_SPECIFIC_PARAMETERS
// structure passed in an ADD PVC request for Q.2931
//
typedef struct _Q2931_ADD_PVC
{
	ATM_ADDRESS					CalledParty;
	ATM_ADDRESS					CallingParty;
	ATM_VPIVCI					ConnectionId;
	ATM_AAL_TYPE				AALType;
	ATM_FLOW_PARAMETERS			ForwardFP;
	ATM_FLOW_PARAMETERS			BackwardFP;
	ULONG						Flags;
	ATM_PVC_SAP					LocalSap;
	ATM_PVC_SAP					DestinationSap;
	BOOLEAN						LIJIdPresent;
	ATM_LIJ_CALLID_IE			LIJId;
} Q2931_ADD_PVC, *PQ2931_ADD_PVC;

//
// These flags are defined to be used with Q2931_ADD_PVC above
//
// this VC should be used by the CallMgr as the signaling VC now
#define CO_FLAG_SIGNALING_VC	0x00000001

//
// use this flag when the VC cannot be used for a MakeCall - incoming call only
// the call mgr can then be optimized not to search these PVCs during make call
// processing.
#define CO_FLAG_NO_DEST_SAP		0x00000002

//
//  Use this flag for a PVC that cannot be used to indicate an incoming call.
//
#define CO_FLAG_NO_LOCAL_SAP	0x00000004

//
// the structure passed in the Parameters field of the CO_SPECIFIC_PARAMETERS
// structure passed in an NDIS_CO_PVC request for Q2931
//
typedef struct _Q2931_DELETE_PVC
{
	ATM_VPIVCI					ConnectionId;
} Q2931_DELETE_PVC, *PQ2931_DELETE_PVC;

typedef struct _CO_GET_CALL_INFORMATION
{
	ULONG						CallInfoType;
	ULONG						CallInfoLength;
	PVOID						CallInfoBuffer;
} CO_GET_CALL_INFORMATION, *PCO_GET_CALL_INFORMATION;

//
// the structure for returning the supported VC rates from the miniport,
// returned in response to OID_ATM_SUPPORTED_VC_RATES
//
typedef struct _ATM_VC_RATES_SUPPORTED
{
	ULONG						MinCellRate;
	ULONG						MaxCellRate;
} ATM_VC_RATES_SUPPORTED, *PATM_VC_RATES_SUPPORTED;

//
//	NDIS_PACKET out of band information for ATM.
//
typedef struct _ATM_AAL_OOB_INFO
{
	ATM_AAL_TYPE		AalType;
	union
	{
		struct _ATM_AAL5_INFO
		{
			BOOLEAN		CellLossPriority;
			UCHAR		UserToUserIndication;
			UCHAR		CommonPartIndicator;
		} ATM_AAL5_INFO;

		struct _ATM_AAL0_INFO
		{
			BOOLEAN		CellLossPriority;
			UCHAR		PayLoadTypeIdentifier;
		} ATM_AAL0_INFO;
	};
} ATM_AAL_OOB_INFO, *PATM_AAL_OOB_INFO;


//
//  Physical Line Speeds in bits/sec.
//
#define ATM_PHYS_RATE_SONET_STS3C						155520000
#define ATM_PHYS_RATE_IBM_25						 	 25600000

//
//  ATM cell layer transfer capacities in bits/sec. This is the throughput
//  available for ATM cells, after allowing for physical framing overhead.
//
#define ATM_CELL_TRANSFER_CAPACITY_SONET_STS3C			149760000
#define ATM_CELL_TRANSFER_CAPACITY_IBM_25			 	 25125926



//
//  User data rate in units of 100 bits/sec. This is returned in response to
//  the OID_GEN_CO_LINK_SPEED query. This is the effective rate of
//  transfer of data available to the ATM layer user, after allowing for
//  the ATM cell header.
//
#define ATM_USER_DATA_RATE_SONET_155					  1356317
#define ATM_USER_DATA_RATE_IBM_25			               227556



//
//  The ATM Service Registry MIB Table is used to locate ATM network
//  services. OID_ATM_GET_SERVICE_ADDRESS is used by clients to access
//  this table.
//

typedef ULONG		ATM_SERVICE_REGISTRY_TYPE;

#define ATM_SERVICE_REGISTRY_LECS		1	// LAN Emulation Configuration Server
#define ATM_SERVICE_REGISTRY_ANS		2	// ATM Name Server

//
//  Structure passed to OID_ATM_GET_SERVICE_ADDRESS.
//
typedef struct _ATM_SERVICE_ADDRESS_LIST
{
	ATM_SERVICE_REGISTRY_TYPE	ServiceRegistryType;
	ULONG						NumberOfAddressesAvailable;
	ULONG						NumberOfAddressesReturned;
	ATM_ADDRESS					Address[1];
} ATM_SERVICE_ADDRESS_LIST, *PATM_SERVICE_ADDRESS_LIST;

#endif	//	_ATM_H_

