function sendTPS(data2)
    % Gửi struct hoặc vector từ Simulink về App thông qua biến trong workspace
    assignin('base', 'sendTPS', data2);
end
 