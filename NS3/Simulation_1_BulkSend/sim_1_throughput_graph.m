clc;
clear all;
close all;

table_1 = readtable('Results/2500_m/Throughput_Calculations.csv');
array_1 = table2array(table_1);

time = array_1(:,1);
throughput_1 = array_1(:,2);
throughput_2 = array_1(:,3);

avg_1 = sum(throughput_1(5:end))/length(throughput_1(5:end));
avg_2 = sum(throughput_2(5:end))/length(throughput_2(5:end));

%Plot Throughput vs Time for all flows

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(time,throughput_1,'-','Color','b','LineWidth',2.5);
plot(time,throughput_2,'-','Color','r','LineWidth',2.5);

xlim([0 40]);

xlabel('Time (Seconds)','FontSize',20);
ylabel('Throughput (Mbps)','FontSize',20);
title('Raw Throughput of TCP vs QUIC Bulksend Applications (RSRP_{dBm} = -98, SINR_{dB} = 25)','FontSize',22);

leg = legend('TCP','QUIC');
leg.FontSize = 18;

hold off;
grid on;

