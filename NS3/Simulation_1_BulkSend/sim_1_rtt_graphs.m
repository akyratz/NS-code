clc;
clear all;
close all;

table_1 = readtable('Results/2500_m/senderTcp-rtt-change.csv');
array_1 = table2array(table_1);

table_2 = readtable('Results/2500_m/senderQUIC-rtt-change7.csv');
array_2 = table2array(table_2);

time_1 = array_1(:,1);
func_1 = array_1(:,3);

time_2 = array_2(:,1);
func_2 = array_2(:,4);

alpha = 0.125;
beta = 0.25;

estimated_tcp = zeros(1, length(func_1));
estimated_quic = zeros(1, length(func_2));

estimated_tcp(1) = func_1(1);
estimated_quic(1) = func_2(1);



for i = 2 : length(func_1)    
    estimated_tcp(i) = (1-alpha)*estimated_tcp(i-1) + alpha*func_1(i);   
end

for i = 2 : length(func_2)    
    estimated_quic(i) = (1-alpha)*estimated_quic(i-1) + alpha*func_2(i);   
end

mean_estimated_tcp = mean(estimated_tcp)*1000;
mean_estimated_quic = mean(estimated_quic)*1000;

std_estimated_tcp = std(estimated_tcp)*1000;
std_estimated_quic = std(estimated_quic)*1000;

%Plot Estimated RTT vs Time

fig1 = figure(1);
fig1.Color = 'w';
ax = gca;
ax.Color = 'w';
ax.LineWidth = 1.2;
ax.GridColor = 'k';
ax.GridAlpha = 0.5;
ax.FontSize = 14;

hold on;

subplot(1,2,1);

%plot(time_1,func_1*1000,'-','Color','b','LineWidth',2.2); %in ms

plot(time_1,estimated_tcp*1000,'-','Color','b','LineWidth',2.2);

xlim([0 40]);

xlabel('Time (Seconds)','FontSize',20);
ylabel('Estimated RTT (ms)','FontSize',20);

title('Estimated RTT of TCP Bulksend Application', 'FontSize', 18);

leg = legend('TCP');
leg.FontSize = 18;

grid on;

subplot(1,2,2);

plot(time_2,estimated_quic*1000,'-','Color','r','LineWidth',2.2);

%plot(time_2,func_2*1000,'-','Color','r','LineWidth',2.2); % in ms

xlim([0 40]);

xlabel('Time (Seconds)','FontSize',20);
ylabel('Estimated RTT (ms)','FontSize',20);

leg = legend('QUIC');
leg.FontSize = 18;

title('Estimated RTT of QUIC Bulksend Application', 'FontSize', 18);

grid on;

hold off;


