import json
import websocket
import subprocess 
import time

def run():
    tmp = input("输入本机IP地址 (例如 123.123.123.123): ")
    subprocess.Popen("run.bat -ResX=800 -ResY=480 -windowed", shell=True) 
    while 1:
        try:
            ws_client = websocket.create_connection("ws://"+tmp+":31245", timeout=3)
            break
        except Exception as e:
            print("connecting to {0}".format(tmp))
            print(e)
            time.sleep(1)
        time.sleep(1)
    with open("./Settings/FPV.json") as f:
        jdata = json.load(f)
        jdata['LocalHostIp'] = tmp
    with open("./Settings/FPV.json", "w") as f:
        f.write(json.dumps(jdata))
    with open("./Settings/RGBD.json") as f:
        jdata = json.load(f)
        jdata['LocalHostIp'] = tmp
    with open("./Settings/RGBD.json", "w") as f:
        f.write(json.dumps(jdata))
    with open("./Settings/Stereo.json") as f:
        jdata = json.load(f)
        jdata['LocalHostIp'] = tmp
    with open("./Settings/Stereo.json", "w") as f:
        f.write(json.dumps(jdata))
    print("{:*^30}".format(""))
    print("{:*^26}".format("选择赛项"))
    print("{:*^26}".format("0.极速穿圈-RGBD"))
    print("{:*^24}".format("1.极速穿圈-双目"))
    print("{:*^26}".format("2.自主飞行-RGBD"))
    print("{:*^24}".format("3.自主飞行-双目"))
    print("{:*^28}".format("4.FPV竞速"))
    print("{:*^30}".format(""))
    while 1:
        tmp = input("输入编号: ")
        if tmp == '0' or tmp == '1' or tmp == '2' or tmp == '3' or tmp == '4':
            break
    if(tmp=='0'):
        ws_client.send(
            json.dumps({
                "command":"switch",
                "map":"stage1",
                "sensor": "RGBD"
            })
        )
        ws_client.recv()
    elif(tmp == '1'):
        ws_client.send(
            json.dumps({
                "command":"switch",
                "map":"stage1",
                "sensor": "Stereo"
            })
        )
    elif(tmp == '2'):
        ws_client.send(
            json.dumps({
                "command":"switch",
                "map":"stage2",
                "sensor": "RGBD"
            })
        )
    elif(tmp == '3'):
        ws_client.send(
            json.dumps({
                "command":"switch",
                "map":"stage2",
                "sensor": "Stereo"
            })
        )
    elif(tmp == '4'):
        ws_client.send(
            json.dumps({
                "command":"switch",
                "map":"stage3",
                "sensor": "FPV"
            })
        )


if __name__=="__main__":
    run()
