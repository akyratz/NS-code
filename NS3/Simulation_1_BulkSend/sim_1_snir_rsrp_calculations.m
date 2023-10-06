clc;
clear all;
close all;

tdfread('Results/1500_m/DlRsrpSinrStats.txt','\t');

total_rsrp_tcp = 0;
total_rsrp_quic = 0;

total_sinr_tcp = 0;
total_sinr_quic = 0;

tcp_samples = 0;
quic_samples = 0;

for i = 43 : length(rsrp)
    
    if (RNTI(i) == 1)
        total_rsrp_tcp = total_rsrp_tcp + rsrp(i);
        total_sinr_tcp = total_sinr_tcp + sinr(i);
        tcp_samples = tcp_samples + 1;
    elseif (RNTI(i) == 2)
        total_rsrp_quic = total_rsrp_quic + rsrp(i);
        total_sinr_quic = total_sinr_quic + sinr(i);
        quic_samples = quic_samples + 1;        
    end
    
end

average_rsrp_tcp = total_rsrp_tcp/tcp_samples;
average_sinr_tcp = total_sinr_tcp/tcp_samples;

average_rsrp_quic = total_rsrp_quic/quic_samples;
average_sinr_quic = total_sinr_quic/quic_samples;

average_rsrp = (average_rsrp_tcp + average_rsrp_quic)/2;
average_sinr = (average_sinr_tcp + average_sinr_quic)/2;

average_rsrp_dBm = 10*log10(average_rsrp*1000);
average_sinr_dB = 10*log10(average_sinr);





