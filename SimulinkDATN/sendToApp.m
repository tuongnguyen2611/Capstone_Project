function sendToApp(data)
    % Gửi struct hoặc vector từ Simulink về App thông qua biến trong workspace
    assignin('base', 'sendData', data);
end
