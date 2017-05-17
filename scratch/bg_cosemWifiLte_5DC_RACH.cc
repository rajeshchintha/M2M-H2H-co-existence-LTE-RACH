       
/* This made by changing bg_cosemWifiLte_5DC.cc for incorporation of Priority aware mac shceduler instead of Round robin
 * Idea is to decrease the P2P delay of Video application by assigning priority to  ami
 * Created by RajeshChintha on 20th April 2017 
 * Refer DungNGuyen report Pg:114 for instructions
 
 */
// Modification in src folder: made by RajeshChintha
       // in src/lte/model/pf-ff-mac-scheduler.cc line: 980
       // in src/lte/model/eps-bearer.cc line: 97 & 113
                          
                   // Replaced the entire scr/lte with lte module from https://github.com/signetlabdei/lena-plus
                   // As priority scheduler is not making any effect to reduce the delay time for video application inspite of adding the dedicated eps-bearer and changing the priority for video, i have decided to use lena+/lte module which deals with RACH for m2m applications.


//If you want to have .csv files for background nodes go to src/gen-udp/model/general-udp-client.cc & src/gen-udp/model/general-udp-server.cc and uncoment the corresponding block comments in GeneralUdpClient::StartApplication (void) & GeneralUdpServer::StartApplication (void) and http-client.cc in src/http/model/ 


#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/csma-module.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/general-udp-client-server-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/http-module.h"
//For COSEM ; added by me
#include "ns3/udp-cosem-client-helper.h"
#include "ns3/udp-cosem-server-helper.h"
#include "ns3/data-concentrator-helper.h"
#include "ns3/demand-response-application-helper.h"
#include "ns3/mdm-application-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"
using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
/*  If you want to have 
 


*/
NS_LOG_COMPONENT_DEFINE ("CosemLteTrafficExample");

int
main (int argc, char *argv[])
{

	
	Config::SetDefault ("ns3::TcpSocket::DelAckTimeout",TimeValue(Seconds (0)));
	Config::SetDefault ("ns3::TcpSocket::SegmentSize",UintegerValue(1404));
	Config::SetDefault ("ns3::TcpSocket::ConnTimeout",TimeValue(Seconds (0.5)));
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue(90000000));
	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue(90000000));
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName("ns3::TcpTahoe")));//commented by me
//	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue(TypeId::LookupByName("ns3::TcpNewReno")));//added by me, another type of congestion control
	

	bool verbose = true;  // For MMS
//**************************************************************************************************
/*  
     These no's are taken from PLC_HSDPA_NS2_English.pdf
     Scenario 1: Bg fixed + AMI Variable 
                     Bg=100;  
                     AMI= 200, 400, 600, 800, 1000 per DC
     Scenario 2: Bg variable + AMI fixed
                     AMI= 100 per DC; 
                     Bg= 10, 50, 100, 150, 200, 220 (Since Bg can't be more than 320 including 5Dc's because of SRSPeriodicity of 320)(Deliberately stopped at 220 because of follwoing errors:
   1. SRSPeriodicity error
   2. Http is giving error beyond 45 nodes as 
       terminate called after throwing an instance of 'std::length_error'
  what():  deque::_M_new_elements_at_front )

The ratio of background nodes as suggested by DungNGuyen is :

 Voice : Video : Gaming : Http : Ftp = 30 : 20 : 20 : 20 : 10

*/
//**************************************************************************************************
	uint16_t nSmNodesDC1 = 100;//Number of SMs 
    uint16_t nSmNodesDC2 = 100;//Number of SMs 
    uint16_t nSmNodesDC3 = 100;//Number of SMs 
    uint16_t nSmNodesDC4 = 100;//Number of SMs 
    uint16_t nSmNodesDC5 = 100;//Number of SMs 
	uint16_t numberOfBgNodes = 10;//Number of BG nodes 10:Voice : Video : Gaming : Http : Ftp = 3 : 2 : 2 : 2 : 1 (Same order follows in Pdcp files also; Confirmed by me) (or Http more than 45 giving error -------> verified by me) (********************* Two contraints for Bg nodes:
//                                                                                           1. no of Bg nodes < =250
//                                                                                            2. no of Http nodes <=40 3.
// 3. Not working for combination Bg=200 & Http= 40; upto Bg=150 & Http=30 is ok, tested by me)
    uint16_t nDcNodes = 5;  //  LTE DC nodes
    uint32_t nCcCsma = 10; // Control Center servers (DR)
//*************************************************************************************************
	double simTime = 20.0;
    double reqSmTime = 2.0;
    double reqDcTime = 5.0;
	double distance = 800.0;
	double requestInterval = 1;
    //===============================  These are all start times ==============================================================
        double bgClientTime = 2.0;

             double client1Time = 0.9;// 0.5 for Bg=100; Sm=200
                                      // 0.5 for Bg=100; Sm=400
                                      // 0.5 for Bg=100; Sm=600
                                      // 0.7 for Bg=100; Sm=800
                                      // 0.9 for Bg=100; Sm=1000
             double client2Time = client1Time + 0.001;
             double client3Time = client1Time +0.002;
             double client4Time = client1Time +0.003;
             double client5Time = client1Time + 0.004;

                      double dc1Time = client1Time;
                      double dc2Time = client2Time;
                      double dc3Time = client3Time;
                      double dc4Time = client4Time;
                      double dc5Time = client5Time;
                                             double mdm1Time = 3.0; // MDM will request DC at (mdm1Time+reqDcTime) sec ie;7s
                                             double mdm2Time = 3.1;
                                             double mdm3Time = 3.2;
                                             double mdm4Time = 3.3;
                                             double mdm5Time = 3.4;
              	double dr1Time = 3.5;
 				double dr2Time = 3.6;
 				double dr3Time = 3.7; 
				double dr4Time = 3.8; 
				double dr5Time = 3.9;
    //=============================================================================================
	std::string output = "bg_sm_traffic";
	int type = 0;
	int mode = 0;
	// Command line arguments
	CommandLine cmd;
	cmd.AddValue("numberOfSmNodes", "Number of SMs", nSmNodesDC1);
	cmd.AddValue("numberOfBgNodes", "Number of BG nodes",numberOfBgNodes);
    cmd.AddValue("nDcNodes", "Number of dc nodes", nDcNodes);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("distance", "Distance between eNBs [m]", distance);
	cmd.AddValue("type", "Type of Requesting", type); // 0 = multicast, 1 = sequential, 2 = reporting
	cmd.AddValue("mode", "Mode 0 = LTE; 1 = Hybrid", mode); //
	cmd.AddValue("requestInterval", "Time between two requests [s]", requestInterval);
	cmd.AddValue("outputfile", "File name of the output trace", output);

	cmd.Parse(argc, argv);
//std::cout << "Upto here it is fine!"<<"\n" ; 

	// For MMS
	if (verbose)
	{
//Use this if u want more info:
/*
       $ export 'NS_LOG=CosemApplicationLayerClient=level_all|prefix_func:CosemApplicationLayerServer=level_all|prefix_func:CosemApplicationsProcessClient=level_all|prefix_func:CosemApplicationsProcessServer=level_all|      
prefix_func:UdpCosemWrapperLayerClient=level_all|prefix_func:UdpCosemWrapperLayerServer=level_all|
prefix_func:UdpCosemServerHelper=level_all|prefix_func:DataConcentratorApplication=level_all|prefix_func:MeterDataManagementApplication=level_all|prefix_func:DemandResponseApplication=level_all|prefix_func'

*/
      LogComponentEnable ("CosemApplicationLayerClient", LOG_LEVEL_INFO);
      LogComponentEnable ("CosemApplicationLayerServer", LOG_LEVEL_INFO);
      LogComponentEnable ("CosemApplicationsProcessClient", LOG_LEVEL_INFO);
      LogComponentEnable ("CosemApplicationsProcessServer", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpCosemWrapperLayerClient", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpCosemWrapperLayerServer", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpCosemServerHelper", LOG_LEVEL_INFO);
		
		LogComponentEnable ("CosemLteTrafficExample", LOG_LEVEL_INFO);

	}

	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> (); 
//	Ptr<EpcHelper> epcHelper = CreateObject<EpcHelper> (); //commented by me
	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();   //added by me from lena-simple-epc.cc  


	lteHelper->SetEpcHelper (epcHelper);
//	lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
    
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");// refer DungNGuyen pg:115
//==========================================  Copied from https://github.com/signetlabdei/lena-plus =======================================
	Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (false));
	Config::SetDefault ("ns3::LteHelper::UseIdealPrach", BooleanValue (false));  
//=====================================================================================================
    
	Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("input-defaults.txt"));
	Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
	Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();

	// parse again so you can override default values from the command line
	cmd.Parse(argc, argv);

	Ptr<Node> pgw = epcHelper->GetPgwNode ();

	NodeContainer lteVoiceUeContainer;
	lteVoiceUeContainer.Create((float)0.3*numberOfBgNodes); // No of nodes are as per DungNGuyen

	NodeContainer lteVideoUeContainer;
	lteVideoUeContainer.Create((float)0.2*numberOfBgNodes);

	NodeContainer lteGamingUeContainer;
	lteGamingUeContainer.Create((float)0.2*numberOfBgNodes);

	NodeContainer lteHttpUeContainer;
	lteHttpUeContainer.Create((float)0.2*numberOfBgNodes);//HTTP working with NS3 3.14 from DungNGuyen

//	NodeContainer lteDcUeContainer;
//	lteDcUeContainer.Create(numberOfDc);

	NodeContainer lteFtpUeContainer;
	lteFtpUeContainer.Create(numberOfBgNodes-lteVoiceUeContainer.GetN()
	-lteVideoUeContainer.GetN()-lteGamingUeContainer.GetN()-lteHttpUeContainer.GetN());
//*************** Remote hosts *****************
	NodeContainer lteVoiceRemoteContainer;
	lteVoiceRemoteContainer.Create(1);

	NodeContainer lteVideoRemoteContainer;
	lteVideoRemoteContainer.Create(1);

	NodeContainer lteGamingRemoteContainer;
	lteGamingRemoteContainer.Create(1);

	NodeContainer lteFtpRemoteContainer;
	lteFtpRemoteContainer.Create(1);

	NodeContainer lteHttpRemoteContainer;
	lteHttpRemoteContainer.Create(lteHttpUeContainer.GetN());
//*************Adding a remote host for SM whick acts MDM and contains COSEM client*************
// Create a single RemoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (1);//COSEM client where as ue are COSEM servers
//***************************************************************
	//Get the Node pointers
	Ptr<Node> lteVoiceRemoteNode = lteVoiceRemoteContainer.Get(0);
	Ptr<Node> lteVideoRemoteNode = lteVideoRemoteContainer.Get(0);
	Ptr<Node> lteGamingRemoteNode = lteGamingRemoteContainer.Get(0);
	Ptr<Node> lteFtpRemoteNode = lteFtpRemoteContainer.Get(0);
//****************pointer for SM remote host*************************
	Ptr<Node> remoteHost = remoteHostContainer.Get (0);
	InternetStackHelper internet;
	internet.Install (remoteHostContainer);
//******************************************************************
	// Install Internet stack on all NodeContainers

	//InternetStackHelper internet;//Commented by me 
	internet.Install (lteVoiceRemoteContainer);
	internet.Install (lteVideoRemoteContainer);
	internet.Install (lteGamingRemoteContainer);
	internet.Install (lteFtpRemoteContainer);
	internet.Install (lteHttpRemoteContainer);

	// Create Point to Point connections between P-GW and all remote hosts
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));

	NetDeviceContainer lteVoiceRemoteDevice = p2ph.Install (pgw, lteVoiceRemoteNode);
	NetDeviceContainer lteVideoRemoteDevice = p2ph.Install (pgw, lteVideoRemoteNode);
	NetDeviceContainer lteGamingRemoteDevice = p2ph.Install (pgw, lteGamingRemoteNode);
	NetDeviceContainer lteFtpRemoteDevice = p2ph.Install (pgw, lteFtpRemoteNode);
	NetDeviceContainer lteHttpRemoteDevice;
	NetDeviceContainer lteHttpPgwDevice;

	for (uint32_t i = 0; i < lteHttpRemoteContainer.GetN(); i++)
	{
		NetDeviceContainer nd = p2ph.Install (pgw, lteHttpRemoteContainer.Get(i));
		lteHttpPgwDevice.Add(nd.Get(0));
		lteHttpRemoteDevice.Add(nd.Get(1));
	}
//****************Internet for sm remote host******************
	NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
	// For COSEM
	Ipv4InterfaceContainer clientLteInterfaces;
	clientLteInterfaces.Add (ipv4h.Assign (internetDevices.Get (1)));
	
//***********************************************************
 
	Ipv4AddressHelper ipv4hVoice;
	ipv4hVoice.SetBase ("2.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer lteVoiceRemoteInterface = ipv4hVoice.Assign (lteVoiceRemoteDevice);
	Ipv4Address lteVoiceRemoteAddress = lteVoiceRemoteInterface.GetAddress (1);//commented by me for debugging on 6th April 12 PM

	Ipv4AddressHelper ipv4hVideo;
	ipv4hVideo.SetBase ("3.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer lteVideoRemoteInterface = ipv4hVideo.Assign (lteVideoRemoteDevice);
	Ipv4Address lteVideoRemoteAddress = lteVideoRemoteInterface.GetAddress (1);//commented by me

	Ipv4AddressHelper ipv4hGaming;
	ipv4hGaming.SetBase ("4.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer lteGamingRemoteInterface = ipv4hGaming.Assign (lteGamingRemoteDevice);
	Ipv4Address lteGamingRemoteAddress = lteGamingRemoteInterface.GetAddress (1);//commented by me

	Ipv4AddressHelper ipv4hFtp;
	ipv4hFtp.SetBase ("5.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer lteFtpRemoteInterface = ipv4hFtp.Assign (lteFtpRemoteDevice);
//	Ipv4Address lteFtpRemoteAddress = lteFtpRemoteInterface.GetAddress (1);

	Ipv4AddressHelper ipv4hHttp;
	ipv4hHttp.SetBase ("16.0.0.0", "255.0.0.0");//"10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer lteHttpRemoteInterface;
	Ipv4InterfaceContainer lteHttpPgwInterface;
	for (uint32_t i = 0; i < lteHttpRemoteDevice.GetN(); i++)
	{
		lteHttpPgwInterface.Add(ipv4hHttp.Assign(lteHttpPgwDevice.Get(i)));
		lteHttpRemoteInterface.Add(ipv4hHttp.Assign(lteHttpRemoteDevice.Get(i)));
		ipv4hHttp.NewNetwork();
	}

	// Install needed routing information from remote hosts to UEs
	Ipv4StaticRoutingHelper ipv4RoutingHelper;

	Ptr<Ipv4StaticRouting> lteVoiceRemoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (lteVoiceRemoteNode->GetObject<Ipv4> ());
	lteVoiceRemoteStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	Ptr<Ipv4StaticRouting> lteVideoRemoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (lteVideoRemoteNode->GetObject<Ipv4> ());
	lteVideoRemoteStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	Ptr<Ipv4StaticRouting> lteGamingRemoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (lteGamingRemoteNode->GetObject<Ipv4> ());
	lteGamingRemoteStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	Ptr<Ipv4StaticRouting> lteFtpRemoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (lteFtpRemoteNode->GetObject<Ipv4> ());
	lteFtpRemoteStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	for (uint32_t i = 0; i < lteHttpRemoteContainer.GetN(); i++)
	{
		Ptr<Ipv4StaticRouting> lteHttpRemoteStaticRouting = ipv4RoutingHelper.GetStaticRouting (lteHttpRemoteContainer.Get(i)->GetObject<Ipv4> ());
		lteHttpRemoteStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
	}
 Ptr<Ipv4StaticRouting> pgwStaticRouting = ipv4RoutingHelper.GetStaticRouting (pgw->GetObject<Ipv4> ());
  pgwStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.0.0.0"), Ipv4Mask ("255.0.0.0"), 2); // Working!!!
//****************Routing for SM remote host********************
	//Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
//**************************************************************

//*******************************Upto here we have created remote hosts and backgroud Ues and installed Ipv4 and made p2p links between pgw and remote hosts**************************************************************************************************************************
	NodeContainer enbNodes;
//	NodeContainer ueNodes;//for SM nodes added by me
    NodeContainer dcNodes;
	enbNodes.Create(1);
//	ueNodes.Create(numberOfSmNodes);//for SM nodes added by me
    dcNodes.Create(nDcNodes);

	// Install Mobility Model
	// Position of eNodeB
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (distance, distance, 0.0));
	MobilityHelper enbMobility;
	enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	enbMobility.SetPositionAllocator (positionAlloc);
	enbMobility.Install (enbNodes);

	// Position of UEs attached to eNodeB
	MobilityHelper uemobility;
	uemobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
			"X", DoubleValue (0.0),
			"Y", DoubleValue (0.0),
			"rho", DoubleValue (distance));


	//	uemobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
	//	                                 "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=200.0]"),"Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=200.0]"));


	uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	uemobility.Install(lteVoiceUeContainer);
	uemobility.Install(lteVideoUeContainer);
	uemobility.Install(lteGamingUeContainer);
	uemobility.Install(lteHttpUeContainer);
	uemobility.Install(lteFtpUeContainer);
//***************Mobily model for SM ues ******************

//	uemobility.Install(ueNodes);
    uemobility.Install(dcNodes);
//*********************************************************

	// Install LTE Devices to the nodes
	NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
	NetDeviceContainer lteVoiceUeDevice = lteHelper->InstallUeDevice (lteVoiceUeContainer);
	NetDeviceContainer lteVideoUeDevice = lteHelper->InstallUeDevice (lteVideoUeContainer);
	NetDeviceContainer lteGamingUeDevice = lteHelper->InstallUeDevice (lteGamingUeContainer);
	NetDeviceContainer lteHttpUeDevice = lteHelper->InstallUeDevice (lteHttpUeContainer);
	NetDeviceContainer lteFtpUeDevice = lteHelper->InstallUeDevice (lteFtpUeContainer);
//****************Install LTE Devices to DC nodes**********
//NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
NetDeviceContainer dcLteDevs = lteHelper->InstallUeDevice (dcNodes);
//************************************************************

	// Install the IP stack on the UEs
	internet.Install(lteVoiceUeContainer);
	internet.Install(lteVideoUeContainer);
	internet.Install(lteGamingUeContainer);
	internet.Install(lteHttpUeContainer);
	internet.Install(lteFtpUeContainer);
//	internet.Install (ueNodes);// for SM ue nodes abbded by me
    internet.Install (dcNodes);// for dc nodes abbded by me

	Ipv4InterfaceContainer lteVoiceUeInterface;
	Ipv4InterfaceContainer lteVideoUeInterface;
	Ipv4InterfaceContainer lteGamingUeInterface;
	Ipv4InterfaceContainer lteHttpUeInterface;
	Ipv4InterfaceContainer lteFtpUeInterface;
//	Ipv4InterfaceContainer ueIpIface;//For SM ue nodes added by me
	Ipv4InterfaceContainer dcIpIface;//For DC nodes added by me

	lteVoiceUeInterface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteVoiceUeDevice));
	lteVideoUeInterface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteVideoUeDevice));
	lteGamingUeInterface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteGamingUeDevice));
	lteHttpUeInterface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteHttpUeDevice));
//	lteDcUeInterface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteDcUeDevice));
	lteFtpUeInterface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteFtpUeDevice));
//	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));//For SM ue nodes added by me
    dcIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (dcLteDevs));//For DC nodes added by me

	// Assign IP address to UEs, and install applications

	for (uint32_t u = 0; u < lteVoiceUeContainer.GetN (); ++u)
	{
		Ptr<Node> ueNode = lteVoiceUeContainer.Get (u);
		Ptr<MobilityModel> mm = ueNode->GetObject<MobilityModel>();
		NS_LOG_INFO("Voice UE Node position " << u << " " << mm->GetPosition().x << " " << mm->GetPosition().y);

		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}

	for (uint32_t u = 0; u < lteVideoUeContainer.GetN (); ++u)
	{
		Ptr<Node> ueNode = lteVideoUeContainer.Get (u);
		Ptr<MobilityModel> mm = ueNode->GetObject<MobilityModel>();
		NS_LOG_INFO("Video UE Node position " << u << " " << mm->GetPosition().x << " " << mm->GetPosition().y);

		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

       
	}
	for (uint32_t u = 0; u < lteGamingUeContainer.GetN (); ++u)
	{
		Ptr<Node> ueNode = lteGamingUeContainer.Get (u);
		Ptr<MobilityModel> mm = ueNode->GetObject<MobilityModel>();
		NS_LOG_INFO("Gaming UE Node position " << u << " " << mm->GetPosition().x << " " << mm->GetPosition().y);

		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}
	for (uint32_t u = 0; u < lteFtpUeContainer.GetN (); ++u)
	{
		Ptr<Node> ueNode = lteFtpUeContainer.Get (u);
		Ptr<MobilityModel> mm = ueNode->GetObject<MobilityModel>();
		NS_LOG_INFO("FTP UE Node position " << u << " " << mm->GetPosition().x << " " << mm->GetPosition().y);

		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}
	for (uint32_t u = 0; u < lteHttpUeContainer.GetN (); ++u)
	{
		Ptr<Node> ueNode = lteHttpUeContainer.Get (u);
		Ptr<MobilityModel> mm = ueNode->GetObject<MobilityModel>();
		NS_LOG_INFO("HTTP UE Node position " << u << " " << mm->GetPosition().x << " " << mm->GetPosition().y);

		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}


//*******************************************************************
//************For DC nodes ***************************************
for (uint32_t u = 0; u < dcNodes.GetN (); ++u)
	{
		Ptr<Node> ueNode = dcNodes.Get (u);
		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}
//*******************************************************************



	// Attach UEs to eNodeB

//	for (uint16_t i = 0; i < lteDcUeContainer.GetN(); i++)
//	{
//		lteHelper->Attach (lteDcUeDevice.Get(i), enbLteDevs.Get(0));
//		//		if (ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetState() == LteUeRrc::CONNECTED_NORMALLY)
//		//		{
//		//
//	}

	for (uint16_t i = 0; i < lteVoiceUeContainer.GetN(); i++)
	{
		lteHelper->Attach (lteVoiceUeDevice.Get(i), enbLteDevs.Get(0));
		//		if (ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetState() == LteUeRrc::CONNECTED_NORMALLY)
		//		{
		//
		//		}
	}

	for (uint16_t i = 0; i < lteVideoUeContainer.GetN(); i++)
	{
		lteHelper->Attach (lteVideoUeDevice.Get(i), enbLteDevs.Get(0));
		//		if (ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetState() == LteUeRrc::CONNECTED_NORMALLY)
		//		{
		//
		//		}
        
	}

	for (uint16_t i = 0; i < lteGamingUeContainer.GetN(); i++)
	{
		lteHelper->Attach (lteGamingUeDevice.Get(i), enbLteDevs.Get(0));
		//		if (ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetState() == LteUeRrc::CONNECTED_NORMALLY)
		//		{
		//
		//		}
	}

	for (uint16_t i = 0; i < lteHttpUeContainer.GetN(); i++)
	{
		lteHelper->Attach (lteHttpUeDevice.Get(i), enbLteDevs.Get(0));

		//		if (ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetState() == LteUeRrc::CONNECTED_NORMALLY)
		//		{
		//
		//		}
	}

	for (uint16_t i = 0; i < lteFtpUeContainer.GetN(); i++)
	{
		lteHelper->Attach (lteFtpUeDevice.Get(i), enbLteDevs.Get(0));
		//		if (ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetState() == LteUeRrc::CONNECTED_NORMALLY)
		//		{
		//
		//		}
	}

//**********Attach DC to eNodeB*********************************

for (uint16_t i = 0; i < nDcNodes; i++)
	{
		lteHelper->Attach (dcLteDevs.Get(i), enbLteDevs.Get(0));
                
	}


// -------------------------------------------------------------------------------
//  CSMA CONTROL CENTER NETWORK CONFIGURATION
// -------------------------------------------------------------------------------

  // CC Nodes creation
  NodeContainer CcCsmaNodes; 
  CcCsmaNodes.Add (remoteHost); // Add the remote Host (GW) node created to the CSMA network
  CcCsmaNodes.Create (nCcCsma);

  // Control Center Links configuration & NetDevice instalation
  CsmaHelper CcCsma;
  CcCsma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  CcCsma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer CcCsmaDevices;
  CcCsmaDevices = CcCsma.Install (CcCsmaNodes);
  
  // Internet Stack instalation (without the remoteHost)
  for (uint32_t i = 1; i < CcCsmaNodes.GetN (); ++i)
     {
        internet.Install (CcCsmaNodes.Get (i));
     }

  // Assign Ip Address
  Ipv4AddressHelper addressCC;
  addressCC.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer CcCsmaInterfaces;
 for (uint32_t i = 0; i < (CcCsmaNodes.GetN ()); ++i)
     {
        CcCsmaInterfaces.Add (addressCC.Assign (CcCsmaDevices.Get (i)));
     }

   // Set the default gateway (Router) for CC server
  for (uint32_t i = 1; i < CcCsmaNodes.GetN (); ++i)
     {
       Ptr<Ipv4StaticRouting> ccStaticRouting = ipv4RoutingHelper.GetStaticRouting (CcCsmaNodes.Get (i)->GetObject<Ipv4> ());
       ccStaticRouting->SetDefaultRoute (CcCsmaInterfaces.GetAddress (0), 1); // Working!!!
    }
// -------------------------------------------------------------------------------
//  WIFI AMI NETWORK CONFIGURATION
// -------------------------------------------------------------------------------
  NodeContainer wifiStaNodesDC1,wifiStaNodesDC2,wifiStaNodesDC3,wifiStaNodesDC4,wifiStaNodesDC5;
  wifiStaNodesDC1.Create (nSmNodesDC1);
  wifiStaNodesDC2.Create (nSmNodesDC2);
  wifiStaNodesDC3.Create (nSmNodesDC3);
  wifiStaNodesDC4.Create (nSmNodesDC4);
  wifiStaNodesDC5.Create (nSmNodesDC5);

  NodeContainer wifiApNodeDC1 = dcNodes.Get (0); // DC also acts s AP
  NodeContainer wifiApNodeDC2 = dcNodes.Get (1);
  NodeContainer wifiApNodeDC3 = dcNodes.Get (2);
  NodeContainer wifiApNodeDC4 = dcNodes.Get (3);
  NodeContainer wifiApNodeDC5 = dcNodes.Get (4);


  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phyDC1 = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper phyDC2 = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper phyDC3 = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper phyDC4 = YansWifiPhyHelper::Default ();
  YansWifiPhyHelper phyDC5 = YansWifiPhyHelper::Default ();


  phyDC1.SetChannel (channel.Create ());
  phyDC2.SetChannel (channel.Create ());
  phyDC3.SetChannel (channel.Create ());
  phyDC4.SetChannel (channel.Create ());
  phyDC5.SetChannel (channel.Create ());


  WifiHelper wifiDC1 = WifiHelper ();
  wifiDC1.SetRemoteStationManager ("ns3::AarfWifiManager");
  WifiHelper wifiDC2 = WifiHelper ();
  wifiDC2.SetRemoteStationManager ("ns3::AarfWifiManager");
  WifiHelper wifiDC3 = WifiHelper ();
  wifiDC3.SetRemoteStationManager ("ns3::AarfWifiManager");
  WifiHelper wifiDC4 = WifiHelper ();
  wifiDC4.SetRemoteStationManager ("ns3::AarfWifiManager");
  WifiHelper wifiDC5 = WifiHelper ();
  wifiDC5.SetRemoteStationManager ("ns3::AarfWifiManager");


  NqosWifiMacHelper macDC1 = NqosWifiMacHelper::Default ();
  NqosWifiMacHelper macDC2 = NqosWifiMacHelper::Default ();
  NqosWifiMacHelper macDC3 = NqosWifiMacHelper::Default ();
  NqosWifiMacHelper macDC4 = NqosWifiMacHelper::Default ();
  NqosWifiMacHelper macDC5 = NqosWifiMacHelper::Default ();

  Ssid ssidDC1 = Ssid ("ns-3-ssid_DC1");
  Ssid ssidDC2 = Ssid ("ns-3-ssid_DC2");
  Ssid ssidDC3 = Ssid ("ns-3-ssid_DC3");  
  Ssid ssidDC4 = Ssid ("ns-3-ssid_DC4");
  Ssid ssidDC5 = Ssid ("ns-3-ssid_DC5");


  macDC1.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidDC1),
               "ActiveProbing", BooleanValue (false));
  macDC2.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidDC2),
               "ActiveProbing", BooleanValue (false));
  macDC3.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidDC3),
               "ActiveProbing", BooleanValue (false));
  macDC4.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidDC4),
               "ActiveProbing", BooleanValue (false));
  macDC5.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssidDC5),
               "ActiveProbing", BooleanValue (false));



  NetDeviceContainer staDevicesDC1,staDevicesDC2,staDevicesDC3,staDevicesDC4,staDevicesDC5;
  staDevicesDC1 = wifiDC1.Install (phyDC1, macDC1, wifiStaNodesDC1);
  staDevicesDC2 = wifiDC2.Install (phyDC2, macDC2, wifiStaNodesDC2);
  staDevicesDC3 = wifiDC3.Install (phyDC3, macDC3, wifiStaNodesDC3);
  staDevicesDC4 = wifiDC4.Install (phyDC4, macDC4, wifiStaNodesDC4);
  staDevicesDC5 = wifiDC5.Install (phyDC5, macDC5, wifiStaNodesDC5);

  macDC1.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssidDC1));
  macDC2.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssidDC2));
  macDC3.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssidDC3));
  macDC4.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssidDC4));
  macDC5.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssidDC5));

  NetDeviceContainer apDevicesDC1, apDevicesDC2,apDevicesDC3,apDevicesDC4,apDevicesDC5;
  apDevicesDC1 = wifiDC1.Install (phyDC1, macDC1, wifiApNodeDC1);
  apDevicesDC2 = wifiDC2.Install (phyDC2, macDC2, wifiApNodeDC2);
  apDevicesDC3 = wifiDC3.Install (phyDC3, macDC3, wifiApNodeDC3);
  apDevicesDC4 = wifiDC4.Install (phyDC4, macDC4, wifiApNodeDC4);
  apDevicesDC5 = wifiDC5.Install (phyDC5, macDC5, wifiApNodeDC5);


  MobilityHelper mobilityWifi;

  /*mobilityWifi.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 //"GridWidth", UintegerValue (3),
                                 "GridWidth", UintegerValue (100), // number of objects layed out on a line.
                                 "LayoutType", StringValue ("RowFirst"));*/

  //mobilityWifi.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //                           "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobilityWifi.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityWifi.Install (wifiStaNodesDC1);
  mobilityWifi.Install (wifiStaNodesDC2);
  mobilityWifi.Install (wifiStaNodesDC3);
  mobilityWifi.Install (wifiStaNodesDC4);
  mobilityWifi.Install (wifiStaNodesDC5);


  mobilityWifi.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityWifi.Install (wifiApNodeDC1);
  mobilityWifi.Install (wifiApNodeDC2);
  mobilityWifi.Install (wifiApNodeDC3);
  mobilityWifi.Install (wifiApNodeDC4);
  mobilityWifi.Install (wifiApNodeDC5);


  //InternetStackHelper stack;
  //internet.Install (wifiApNode);
  internet.Install (wifiStaNodesDC1);
  internet.Install (wifiStaNodesDC2);
  internet.Install (wifiStaNodesDC3);
  internet.Install (wifiStaNodesDC4);
  internet.Install (wifiStaNodesDC5);


  Ipv4AddressHelper address;
  Ipv4InterfaceContainer serverInterfacesDC1, serverInterfacesDC2,serverInterfacesDC3,serverInterfacesDC4,serverInterfacesDC5;

  address.SetBase ("200.0.0.0", "255.252.0.0");
  // Ipv4InterfaceContainer wifiInterfaces;
  // address.Assign (staDevices);
 
  Ipv4InterfaceContainer    dcWifiInterfacesDC1,dcWifiInterfacesDC2,dcWifiInterfacesDC3,dcWifiInterfacesDC4,dcWifiInterfacesDC5;
  dcWifiInterfacesDC1.Add (address.Assign (apDevicesDC1));
  dcWifiInterfacesDC2.Add (address.Assign (apDevicesDC2));
  dcWifiInterfacesDC3.Add (address.Assign (apDevicesDC3));
  dcWifiInterfacesDC4.Add (address.Assign (apDevicesDC4));
  dcWifiInterfacesDC5.Add (address.Assign (apDevicesDC5));

//  address.SetBase ("75.0.0.0", "255.252.0.0");
  for (uint32_t i = 0; i < wifiStaNodesDC1.GetN (); ++i)
     {
        serverInterfacesDC1.Add (address.Assign (staDevicesDC1.Get (i)));
     }
// address.SetBase ("80.0.0.0", "255.252.0.0");
for (uint32_t i = 0; i < wifiStaNodesDC2.GetN (); ++i)
     {
        serverInterfacesDC2.Add (address.Assign (staDevicesDC2.Get (i)));
     }
//address.SetBase ("85.0.0.0", "255.252.0.0");
for (uint32_t i = 0; i < wifiStaNodesDC3.GetN (); ++i)
     {
        serverInterfacesDC3.Add (address.Assign (staDevicesDC3.Get (i)));
     }
//address.SetBase ("90.0.0.0", "255.252.0.0");
for (uint32_t i = 0; i < wifiStaNodesDC4.GetN (); ++i)
     {
        serverInterfacesDC4.Add (address.Assign (staDevicesDC4.Get (i)));
     }
//address.SetBase ("95.0.0.0", "255.252.0.0");
for (uint32_t i = 0; i < wifiStaNodesDC5.GetN (); ++i)
     {
        serverInterfacesDC5.Add (address.Assign (staDevicesDC5.Get (i)));
     }

  //Ipv4InterfaceContainer apInterfaces;
  //apInterfaces.Add (address.Assign (apDevices));

//

// -------------------------------------------------------------------------------
//  COSEM & DATA CONCENTRATOR APPLICATIONS CONFIGURATION
// -------------------------------------------------------------------------------

 // Cosem Application

//COSEM Servers for DC1
  UdpCosemServerHelper cosemServerDC1 (serverInterfacesDC1);
  ApplicationContainer serverAppsDC1 = cosemServerDC1.Install (wifiStaNodesDC1);
  serverAppsDC1.Start (Seconds (0.0001));
  serverAppsDC1.Stop (Seconds (simTime));//10.0));

//COSEM Servers for DC2
  UdpCosemServerHelper cosemServerDC2 (serverInterfacesDC2);
  ApplicationContainer serverAppsDC2 = cosemServerDC2.Install (wifiStaNodesDC2);
  serverAppsDC2.Start (Seconds (0.0001));
  serverAppsDC2.Stop (Seconds (simTime));//10.0));

//COSEM Servers for DC3
  UdpCosemServerHelper cosemServerDC3 (serverInterfacesDC3);
  ApplicationContainer serverAppsDC3 = cosemServerDC3.Install (wifiStaNodesDC3);
  serverAppsDC3.Start (Seconds (0.0001));
  serverAppsDC3.Stop (Seconds (simTime));//10.0));

//COSEM Servers for DC4
  UdpCosemServerHelper cosemServerDC4 (serverInterfacesDC4);
  ApplicationContainer serverAppsDC4 = cosemServerDC4.Install (wifiStaNodesDC4);
  serverAppsDC4.Start (Seconds (0.0001));
  serverAppsDC4.Stop (Seconds (simTime));//10.0));

//COSEM Servers for DC5
  UdpCosemServerHelper cosemServerDC5 (serverInterfacesDC5);
  ApplicationContainer serverAppsDC5 = cosemServerDC5.Install (wifiStaNodesDC5);
  serverAppsDC5.Start (Seconds (0.0001));
  serverAppsDC5.Stop (Seconds (simTime));//10.0));


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%        COSEM Clients (DCs)    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 //DC1
  UdpCosemClientHelper cosemClient (serverAppsDC1, dcWifiInterfacesDC1, Seconds (reqSmTime));//requestInterval));//5.0));//modified by me
  ApplicationContainer clientApps = cosemClient.Install (dcNodes.Get (0));
  clientApps.Start (Seconds (client1Time));
  clientApps.Stop (Seconds (simTime));

//DC2
  UdpCosemClientHelper cosemClient2 (serverAppsDC2, dcWifiInterfacesDC2, Seconds (reqSmTime));//requestInterval));//5.0));//modified by me
  ApplicationContainer clientApps2 = cosemClient2.Install (dcNodes.Get (1));
  clientApps2.Start (Seconds (client2Time));
  clientApps2.Stop (Seconds (simTime));

//DC3
  UdpCosemClientHelper cosemClient3 (serverAppsDC3, dcWifiInterfacesDC3, Seconds (reqSmTime));//requestInterval));//5.0));//modified by me
  ApplicationContainer clientApps3 = cosemClient3.Install (dcNodes.Get (2));
  clientApps3.Start (Seconds (client3Time));
  clientApps3.Stop (Seconds (simTime));

//DC4
  UdpCosemClientHelper cosemClient4 (serverAppsDC4, dcWifiInterfacesDC4, Seconds (reqSmTime));//requestInterval));//5.0));//modified by me
  ApplicationContainer clientApps4 = cosemClient4.Install (dcNodes.Get (3));
  clientApps4.Start (Seconds (client4Time));
  clientApps4.Stop (Seconds (simTime));

//DC5
  UdpCosemClientHelper cosemClient5 (serverAppsDC5, dcWifiInterfacesDC5, Seconds (reqSmTime));//requestInterval));//5.0));//modified by me
  ApplicationContainer clientApps5 = cosemClient5.Install (dcNodes.Get (4));
  clientApps5.Start (Seconds (client5Time));
  clientApps5.Stop (Seconds (simTime));




// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    Data Concentrator Application   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//DC1
  DataConcentratorApplicationHelper dc (clientApps, CcCsmaInterfaces.GetAddress (1), dcIpIface.GetAddress (0)); //ApplicationContainer clientApps, Address centerIp, Address dcIpAddress
  ApplicationContainer dcApps = dc.Install (dcNodes.Get (0));
  dcApps.Start (Seconds (dc1Time));
  dcApps.Stop (Seconds (simTime));

//DC2
  DataConcentratorApplicationHelper dc2 (clientApps2, CcCsmaInterfaces.GetAddress (2), dcIpIface.GetAddress (1)); //ApplicationContainer clientApps, Address centerIp, Address dcIpAddress
  ApplicationContainer dcApps2 = dc2.Install (dcNodes.Get (1));
  dcApps2.Start (Seconds (dc2Time));
  dcApps2.Stop (Seconds (simTime));

//DC3
  DataConcentratorApplicationHelper dc3 (clientApps3, CcCsmaInterfaces.GetAddress (3), dcIpIface.GetAddress (2)); //ApplicationContainer clientApps, Address centerIp, Address dcIpAddress
  ApplicationContainer dcApps3 = dc3.Install (dcNodes.Get (2));
  dcApps3.Start (Seconds (dc3Time));
  dcApps3.Stop (Seconds (simTime));

//DC4
  DataConcentratorApplicationHelper dc4 (clientApps4, CcCsmaInterfaces.GetAddress (4), dcIpIface.GetAddress (3)); //ApplicationContainer clientApps, Address centerIp, Address dcIpAddress
  ApplicationContainer dcApps4 = dc4.Install (dcNodes.Get (3));
  dcApps4.Start (Seconds (dc4Time));
  dcApps4.Stop (Seconds (simTime));

//DC5
  DataConcentratorApplicationHelper dc5 (clientApps5, CcCsmaInterfaces.GetAddress (5), dcIpIface.GetAddress (4)); //ApplicationContainer clientApps, Address centerIp, Address dcIpAddress
  ApplicationContainer dcApps5 = dc5.Install (dcNodes.Get (4));
  dcApps5.Start (Seconds (dc5Time));
  dcApps5.Stop (Seconds (simTime));

// -------------------------------------------------------------------------------
//  METER DATA MANAGEMENT & DEMAND RESPONSE APPLICATIONS CONFIGURATION 
// -------------------------------------------------------------------------------
 
  // Meter Data Management Application on Control Center

//MD1
  MeterDataManagementApplicationHelper mdmHelper (dcApps, CcCsmaInterfaces.GetAddress (1), Seconds (reqDcTime), reqSmTime);//ApplicationContainer dataConcentratorAppContainer, Address mdmIp, Time interval, uint32_t readingTime
  ApplicationContainer mdmApps = mdmHelper.Install (CcCsmaNodes.Get (1)); 
  mdmApps.Start (Seconds (mdm1Time));
  mdmApps.Stop (Seconds (simTime));

//MD2
  MeterDataManagementApplicationHelper mdmHelper2 (dcApps2, CcCsmaInterfaces.GetAddress (2), Seconds (reqDcTime), reqSmTime);
  ApplicationContainer mdmApps2 = mdmHelper2.Install (CcCsmaNodes.Get (2)); 
  mdmApps2.Start (Seconds (mdm2Time));
  mdmApps2.Stop (Seconds (simTime));

//MD3
  MeterDataManagementApplicationHelper mdmHelper3 (dcApps3, CcCsmaInterfaces.GetAddress (3), Seconds (reqDcTime), reqSmTime);
  ApplicationContainer mdmApps3 = mdmHelper3.Install (CcCsmaNodes.Get (3)); 
  mdmApps3.Start (Seconds (mdm3Time));
  mdmApps3.Stop (Seconds (simTime));

//MD4
  MeterDataManagementApplicationHelper mdmHelper4 (dcApps4, CcCsmaInterfaces.GetAddress (4), Seconds (reqDcTime), reqSmTime);
  ApplicationContainer mdmApps4 = mdmHelper4.Install (CcCsmaNodes.Get (4)); 
  mdmApps4.Start (Seconds (mdm4Time));
  mdmApps4.Stop (Seconds (simTime));

//MD5
  MeterDataManagementApplicationHelper mdmHelper5 (dcApps5, CcCsmaInterfaces.GetAddress (5), Seconds (reqDcTime), reqSmTime);
  ApplicationContainer mdmApps5 = mdmHelper5.Install (CcCsmaNodes.Get (5)); 
  mdmApps5.Start (Seconds (mdm5Time));
  mdmApps5.Stop (Seconds (simTime));


  // Demand Response Application on Control Center (Send demand response signals to Data Cocentrator)
//DR1
  DemandResponseApplicationHelper drHelper (CcCsmaInterfaces.GetAddress (6), mdmApps);
  ApplicationContainer drApps = drHelper.Install (CcCsmaNodes.Get (6));
  drApps.Start (Seconds (dr1Time));
  drApps.Stop (Seconds (simTime));

//DR2
  DemandResponseApplicationHelper drHelper2 (CcCsmaInterfaces.GetAddress (7), mdmApps2);
  ApplicationContainer drApps2 = drHelper2.Install (CcCsmaNodes.Get (7));
  drApps2.Start (Seconds (dr2Time));
  drApps2.Stop (Seconds (simTime));

//DR3
  DemandResponseApplicationHelper drHelper3 (CcCsmaInterfaces.GetAddress (8), mdmApps3);
  ApplicationContainer drApps3 = drHelper3.Install (CcCsmaNodes.Get (8));
  drApps3.Start (Seconds (dr3Time));
  drApps3.Stop (Seconds (simTime));

//DR4
  DemandResponseApplicationHelper drHelper4 (CcCsmaInterfaces.GetAddress (9), mdmApps4);
  ApplicationContainer drApps4 = drHelper4.Install (CcCsmaNodes.Get (9));
  drApps4.Start (Seconds (dr4Time));
  drApps4.Stop (Seconds (simTime));

//DR5
  DemandResponseApplicationHelper drHelper5 (CcCsmaInterfaces.GetAddress (10), mdmApps5);
  ApplicationContainer drApps5 = drHelper5.Install (CcCsmaNodes.Get (10));
  drApps5.Start (Seconds (dr5Time));
  drApps5.Stop (Seconds (simTime));

//***********************************This bllock is commented for debug by me*************************************


//  ------------------------------------------------------------------------------------------------------------


	// ------------------------------------------------------------------------------------------------

	// VoIP Application, both UL and DL

	// UPLINK (from UEs) !!!!
	//
	// Create one udpServer applications on node one.
	//
	uint16_t lteVoiceRemotePort = 4000;
	GeneralUdpServerHelper lteVoiceRemoteServer (lteVoiceRemotePort, 3);//VoIP=3
	ApplicationContainer lteVoiceRemoteApp = lteVoiceRemoteServer.Install (lteVoiceRemoteNode);
	lteVoiceRemoteApp.Start (Seconds (0.0));
	lteVoiceRemoteApp.Stop (Seconds (simTime));

	//
	// Create one VoIPClient application to send UDP datagrams from UE nodes to
	// Remote VoIP host.
	//
	GeneralUdpClientHelper client (lteVoiceRemoteAddress, lteVoiceRemotePort, 3); //0 = video; 1 = gaming uplink
	lteVoiceRemoteApp = client.Install (lteVoiceUeContainer);
	lteVoiceRemoteApp.Start (Seconds (bgClientTime));
	lteVoiceRemoteApp.Stop (Seconds (simTime));

	// DOWNLINK (to UEs) !!!!
	//
	// Create udpServer applications on UE nodes.
	//
	uint16_t lteVoiceUePort = 4000;
	GeneralUdpServerHelper lteVoiceUeServer (lteVoiceUePort, 3);
	ApplicationContainer lteVoiceUeApp = lteVoiceUeServer.Install (lteVoiceUeContainer);
	lteVoiceUeApp.Start (Seconds (0.0));
	lteVoiceUeApp.Stop (Seconds (simTime));

	//
	// Create one VoIPClient application to send UDP datagrams from Remote Host to
	// VoIP UEs.
	//
	for (uint32_t i = 0; i < lteVoiceUeInterface.GetN(); i++)
	{
		GeneralUdpClientHelper remoteClient (lteVoiceUeInterface.GetAddress(i), lteVoiceUePort, 3); //0 = video; 1 = gaming uplink
		lteVoiceRemoteApp = remoteClient.Install (lteVoiceRemoteNode);
		lteVoiceRemoteApp.Start (Seconds (bgClientTime));
		lteVoiceRemoteApp.Stop (Seconds (simTime));
	}


	// ------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------

	// Gaming Application, both UL and DL

	// UPLINK (from UEs) !!!!
	//
	// Create one udpServer applications on node one.
	//
	uint16_t lteGamingRemotePort = 5000;
	GeneralUdpServerHelper lteGamingRemoteServer (lteGamingRemotePort, 1);//Gaming uplink=1; Gaming downlink=2
	ApplicationContainer lteGamingUeApp = lteGamingRemoteServer.Install (lteGamingRemoteNode);
	lteGamingUeApp.Start (Seconds (0.0));
	lteGamingUeApp.Stop (Seconds (simTime));

	//
	// Create one VoIPClient application to send UDP datagrams from UE nodes to
	// Remote VoIP host.
	//
	GeneralUdpClientHelper gamingClientUe (lteGamingRemoteAddress, lteGamingRemotePort, 1); //0 = video; 1 = gaming uplink
	lteGamingUeApp = gamingClientUe.Install (lteGamingUeContainer);
	lteGamingUeApp.Start (Seconds (bgClientTime));
	lteGamingUeApp.Stop (Seconds (simTime));

	// DOWNLINK (to UEs) !!!!
	//
	// Create udpServer applications on UE nodes.
	//
	uint16_t lteGamingUePort = 5000;
	GeneralUdpServerHelper lteGamingUeServer (lteGamingUePort, 2);
	ApplicationContainer lteGamingRemoteApp = lteGamingUeServer.Install (lteGamingUeContainer);
	lteGamingRemoteApp.Start (Seconds (0.0));
	lteGamingRemoteApp.Stop (Seconds (simTime));

	//
	// Create one VoIPClient application to send UDP datagrams from Remote Host to
	// VoIP UEs.
	//
	for (uint32_t i = 0; i < lteGamingUeInterface.GetN(); i++)
	{
		GeneralUdpClientHelper gamingClientRemote (lteGamingUeInterface.GetAddress(i), lteGamingUePort, 2); //0 = video; 1 = gaming uplink
		lteGamingRemoteApp = gamingClientRemote.Install (lteGamingRemoteNode);
		lteGamingRemoteApp.Start (Seconds (bgClientTime));
		lteGamingRemoteApp.Stop (Seconds (simTime));
	}


	// ------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------

	// Video Application, both UL and DL

	// UPLINK (from UEs) !!!!
	//
	// Create one udpServer applications on node one.
	//
	uint16_t lteVideoRemotePort = 5000;
	GeneralUdpServerHelper lteVideoRemoteServer (lteVideoRemotePort, 0);//Video=0
	ApplicationContainer lteVideoUeApp = lteVideoRemoteServer.Install (lteVideoRemoteNode);
	lteVideoUeApp.Start (Seconds (0.0));
	lteVideoUeApp.Stop (Seconds (1000.0));

	//
	// Create one VoIPClient application to send UDP datagrams from UE nodes to
	// Remote VoIP host.
	//
	GeneralUdpClientHelper VideoClientUe (lteVideoRemoteAddress, lteVideoRemotePort, 0); //0 = video;
	lteVideoUeApp = VideoClientUe.Install (lteVideoUeContainer);
	lteVideoUeApp.Start (Seconds (bgClientTime));
	lteVideoUeApp.Stop (Seconds (simTime));

	// DOWNLINK (to UEs) !!!!
	//
	// Create udpServer applications on UE nodes.
	//
	uint16_t lteVideoUePort = 5000;
	GeneralUdpServerHelper lteVideoUeServer (lteVideoUePort, 0);
	ApplicationContainer lteVideoRemoteApp = lteVideoUeServer.Install (lteVideoUeContainer);
	lteVideoRemoteApp.Start (Seconds (0.0));
	lteVideoRemoteApp.Stop (Seconds (1000.0));

	//
	// Create one VoIPClient application to send UDP datagrams from Remote Host to
	// VoIP UEs.
	//
	for (uint32_t i = 0; i < lteVideoUeInterface.GetN(); i++)
	{
		GeneralUdpClientHelper VideoClientRemote (lteVideoUeInterface.GetAddress(i), lteVideoUePort, 0); //0 = video; 1 = Video uplink
		lteVideoRemoteApp = VideoClientRemote.Install (lteVideoRemoteNode);
		lteVideoRemoteApp.Start (Seconds (bgClientTime));
		lteVideoRemoteApp.Stop (Seconds (simTime));
	}


	// ------------------------------------------------------------------------------------------------
	// FTP DOWNLINK

	uint16_t port = 9;  // well-known echo port number
	//
	// Create a PacketSinkApplication and install it on node 1
	//
	PacketSinkHelper sink ("ns3::TcpSocketFactory",
			InetSocketAddress (Ipv4Address::GetAny (), port));
	ApplicationContainer sinkApps = sink.Install (lteFtpUeContainer);
	sinkApps.Start (Seconds (0.0));
	sinkApps.Stop (Seconds (1000.0));

	//
	// Create a BulkSendApplication and install it on remote FTP hosts
	//

	for (uint32_t i = 0; i < lteFtpUeInterface.GetN(); i++)
	{
		BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (lteFtpUeInterface.GetAddress(i), port));
		ApplicationContainer sourceApps = source.Install (lteFtpRemoteNode);
		sourceApps.Start (Seconds (bgClientTime));
		sourceApps.Stop (Seconds (simTime));
	}


//	// FTP UPLINK
//
//	//	// UPLINK (from UEs) !!!!
//	//
//	// Create a PacketSinkApplication and install it on Remote FTP Host
//	//
//	PacketSinkHelper sinkRemote ("ns3::TcpSocketFactory",
//			InetSocketAddress (Ipv4Address::GetAny (), port));
//	ApplicationContainer sinkAppsRemote = sinkRemote.Install (lteFtpRemoteNode);
//	sinkAppsRemote.Start (Seconds (0.0));
//	sinkAppsRemote.Stop (Seconds (1000.0));
//
//	//
//	// Create a BulkSendApplication and install it on UEs
//	//
//
//	BulkSendHelper sourceUe ("ns3::TcpSocketFactory", InetSocketAddress (lteFtpRemoteAddress, port));
//	ApplicationContainer sourceAppsUe = sourceUe.Install (lteFtpUeContainer);
//	sourceAppsUe.Start (Seconds (0.1));
//	sourceAppsUe.Stop (Seconds (100.0));



	// ------------------------------------------------------------------------------------------------
	// HTTP Client and Server

	port = 80;		

	for (uint32_t i = 0; i < lteHttpRemoteContainer.GetN(); i++)
	{
		
		HttpHelper httpHelper;

		HttpServerHelper httpServer;
		httpServer.SetAttribute ("Local", AddressValue (InetSocketAddress (Ipv4Address::GetAny (), port)));
		httpServer.SetAttribute ("HttpController", PointerValue (httpHelper.GetController ()));
		ApplicationContainer serverApps = httpServer.Install (lteHttpRemoteContainer.Get (i));

		serverApps.Start (Seconds (0.0));
		serverApps.Stop (Seconds (1000));

		HttpClientHelper httpClient;
		httpClient.SetAttribute ("Peer", AddressValue (InetSocketAddress (lteHttpRemoteInterface.GetAddress (i), port)));
		httpClient.SetAttribute ("HttpController", PointerValue (httpHelper.GetController ()));
//std::cout << "Upto here it is fine!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! "<<"\n" ;
		ApplicationContainer clientApps = httpClient.Install (lteHttpUeContainer.Get(i));

		clientApps.Start (Seconds (bgClientTime));
		clientApps.Stop (Seconds (simTime));
	}


/*  commented by me

//	lteHelper->EnableTraces ();//For PDCP stats refer to lena-simple-epc.cc by RajeshChintha 2Jan2017

	//LTE QoS bearer            //Added by RajeshChintha from scrach/lte-example-sim.cc
//       	EpsBearer bearer (EpsBearer::NGBR_VOICE_VIDEO_GAMING);//Added by RajeshChintha from scrach/lte-example-sim.cc

//for (uint16_t i = 0; i < lteVoiceUeContainer.GetN(); i++)
//	{
//		lteHelper->ActivateDedicatedEpsBearer (lteVoiceUeDevice.Get(i), bearer, EpcTft::Default ()); 

		//		if (ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ()->GetRrc()->GetState() == LteUeRrc::CONNECTED_NORMALLY)
		//		{
		//
		//		}
//	}
    	 
*///Up to here commented by me

/*


//==========================// Dedicated bearer for Video application  //copied from Dung Nguyen pg:113==================================
      
      //define Qci type --> priority = 1; For this you need to change the priority in src/lte.model/eps-bearer.cc line :97 & 113
     for (uint32_t u = 0; u < lteVideoUeContainer.GetN(); u++)
        {

             //Added by me
   //     NS_LOG_INFO( "Hi i am in dedicated bearer for video app");
			enum EpsBearer::Qci q1 = EpsBearer::GBR_NON_CONV_VIDEO;// guarenteed bit rate non conversational //define Qci type --> priority = 1
           // Use GBR_CONV_VIDEO   for video application 
           

			GbrQosInformation qos1;
			EpsBearer bearer1 (q1, qos1);
       //      uint8_t p= bearer1.GetPriority ();
  //      NS_LOG_INFO("The value of priority is: "<< p);
			Ptr<EpcTft> tft1 = Create<EpcTft> ();
			EpcTft::PacketFilter ulpf1;
			tft1->Add(ulpf1);
			lteHelper->ActivateDedicatedEpsBearer(lteVideoUeDevice.Get(u), bearer1,tft1);
       }

//=============================================================

//

//==========================// Dedicated bearer for DC //copied from Dung Nguyen pg:113==================================
      
      //define Qci type --> GBR_MTC_TIME_CRITICAL_FIRST < GBR_NON_CONV_VIDEO
     for (uint32_t u = 0; u < dcNodes.GetN(); u++)
        {
			enum EpsBearer::Qci q2 = EpsBearer::GBR_MTC_TIME_CRITICAL_FIRST; //define Qci type --> priority = 8
           // Use GBR_CONV_VIDEO   for video application 
			GbrQosInformation qos2;
			EpsBearer bearer2 (q2, qos2);
			Ptr<EpcTft> tft2 = Create<EpcTft> ();
			EpcTft::PacketFilter ulpf2;
			tft2->Add(ulpf2);
			lteHelper->ActivateDedicatedEpsBearer(dcLteDevs.Get(u), bearer2,tft2);
       }
//=============================================================

*/


//lteHelper->EnableTraces ();//added by me; copied from just above EpsBearer
lteHelper->EnablePdcpTraces ();//Copied from https://www.nsnam.org/docs/models/html/lte-user.html#basic-simulation-program   by me

Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats (); // Taken from https://www.nsnam.org/docs/models/html/lte-user.html
  pdcpStats->SetAttribute ("StartTime", TimeValue (Seconds (0)));
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (simTime)));

/* *****************************************Post-processing on the produced simulation output ****************************************
	//This section is added by RajeshChintha on 2Jan2017 
                              
		-------------------Troughput calculation in GNU Octave:  http://lena.cttc.es/manual/lte-user.html
       
	% RxBytes is the 10th column
	DlRxBytes = load ("no-op-DlRlcStats.txt") (:,10);
	DlAverageThroughputKbps = sum (DlRxBytes) * 8 / 1000 / 50;// 8 is for bits conversion; 1000 is Kilo; 50 is for average calculation

                                
                 ----------------------SINR calculation:----------------------------------
	% Sinr is the 6th column
	DlSinr = load ("no-op-DlRsrpSinrStats.txt") (:,6);
	% eliminate NaN values
	idx = isnan (DlSinr);
	DlSinr (idx) = 0;
	DlAverageSinrDb = 10 * log10 (mean (DlSinr)) % convert to dB

***************************************************************************************************************************************/


//	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	// Uncomment to enable PCAP tracing
//	p2ph.EnablePcapAll(output);// commented by me *************************************

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	Simulator::Destroy();
	return 0;

}

