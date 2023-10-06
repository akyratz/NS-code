clc;
clear all;
close all;

throughput_tcp = [2.7504 2.8855 2.9381 1.5235]; %fix avg throughput values
throughput_quic = [5.7696 5.4658 3.7887 1.3362];

%Plot Throughput vs Signal Quality

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

plot(throughput_tcp,'--o','Color','b','MarkerSize',12,'MarkerFaceColor','b','LineWidth',2.5);
plot(throughput_quic,'--o','Color','r','MarkerSize',12,'MarkerFaceColor','r','LineWidth',2.5);

xticks([1 2 3 4]);
xticklabels({'(-73.8, 49)','(-85.3, 38)', '(-92.6, 30)', '(-98.0, 25)'});

yticks([1.5 2 2.5 3 3.5 4 4.5 5 5.5 6]);
 
xlabel('(RSRP_{dBm}, SINR_{dB})','FontSize',20);
ylabel('Throughput (Mbps)','FontSize',20);
title('Average Steady State Throughput Performance of TCP vs QUIC under different radio signal conditions over LTE','FontSize',20);

leg = legend('TCP', 'QUIC');
leg.FontSize = 18;

hold off;
grid on;

