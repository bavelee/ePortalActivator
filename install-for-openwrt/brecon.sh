#!/bin/sh

# 无网络时进行重新连接
on_network_not_available(){
echo Network not available $(date +%s) >> /tmp/brecon.log
/etc/init.d/epact-init restart
check_status 1
}

check_network(){
captiveReturnCode=`curl -s -I -m 10 -o /dev/null -s -w %{http_code} http://www.google.cn/generate_204`
if [ "$captiveReturnCode" = "204" ]; then
  return 0
fi
return 1
}
check_status(){
if check_network                                       
then                                                   
        echo -e "[\033[32m成功\033[0m]"
else                                      
        echo -e "[\033[31m失败\033[0m]"
        [[ -z $1 ]] && on_network_not_available
fi
}

echo -n "检测网络是否畅通..."
check_status