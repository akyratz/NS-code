clc;
clear all;
close all;

table_1 = readtable('Results/2500_m/senderTcp-cwnd-change.csv');
array_1 = table2array(table_1);

table_2 = readtable('Results/2500_m/senderQUIC-cwnd-change7.csv');
array_2 = table2array(table_2);

time_1 = array_1(:,1);
func_1 = array_1(:,2);

time_2 = array_2(:,1);
func_2 = array_2(:,2);

%Plot Cwnd vs Time

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(time_1,func_1/1000,'-','Color','b','LineWidth',2.2); %in kBytes
plot(time_2,func_2/1000,'-','Color','r','LineWidth',2.2);

xlim([0 40]);

xlabel('Time (Seconds)','FontSize',20);
ylabel('Congestion Window (kB)','FontSize',20);
title('Congestion Window of TCP vs QUIC Bulksend Applications (RSRP_{dBm} = -98, SINR_{dB} = 25)','FontSize',22);

leg = legend('TCP','QUIC');
leg.FontSize = 18;

hold off;
grid on;

