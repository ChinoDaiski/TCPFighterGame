import os
import re
import sys
import time

import chardet

def read_file_with_detected_encoding(filename):
    # 인코딩을 감지하기 위해 파일을 바이너리 모드로 읽기
    with open(filename, 'rb') as file:
        raw_data = file.read()
        result = chardet.detect(raw_data)
        encoding = result['encoding']
    
    # 감지된 인코딩으로 파일을 다시 읽기
    with open(filename, 'r', encoding=encoding) as file:
        content = file.read()
    
    return content

def parse_txt_file(filename):
    # 현재 스크립트 파일의 디렉토리 경로를 얻습니다.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    file_path = os.path.join(script_dir, filename)
    
    try:
        content = read_file_with_detected_encoding(file_path)

    except FileNotFoundError:
        print(f"'{file_path}' 경로에 있는 파일 '{filename}'을(를) 찾을 수 없습니다.")
        input("press enter to continue...")
        return [], [], {}
    
    # 패턴 정의
    packet_pattern = re.compile(r'#define\s+dfPACKET_(\w+)\s+(\d+)')
    struct_pattern = re.compile(r'/\*\s*([\s\S]*?)\s*\*/')

    # 패킷과 구조체 추출
    packet_matches = packet_pattern.findall(content)
    struct_matches = struct_pattern.findall(content)

    proxy_packets = []
    stub_packets = []
    packet_type_enum = {}
    
    for (packet_name, packet_id), struct_def in zip(packet_matches, struct_matches):
        # 구조체에서 변수 추출
        struct_def_lines = struct_def.strip().split('\n')
        fields = []
        field_names = []
        for line in struct_def_lines:
            if line.strip():
                parts = line.strip().split()
                field_type = parts[0]
                field_name = parts[1]
                field_names.append(field_name)
                fields.append(f'{field_type} {field_name}')
        field_definitions = ', '.join(fields)
        
        # 패킷 정보에 따라 추가
        if packet_name.startswith('SC_'):
            proxy_packets.append({
                'name': packet_name,
                'define': f'dfPACKET_{packet_name}',
                'fields': field_definitions
            })
            packet_type_enum[int(packet_id)] = packet_name
        elif packet_name.startswith('CS_'):
            stub_packets.append({
                'name': packet_name,
                'define': f'dfPACKET_{packet_name}',
                'fields': field_definitions,
                'field_names': ', '.join(field_names)
            })
            packet_type_enum[int(packet_id)] = packet_name
    
    return proxy_packets, stub_packets, packet_type_enum

def generate_proxy_header_file(packets, output_filename):
    with open(output_filename, 'w', encoding='utf-8') as file:
        file.write('#include "Session.h"\n')
        file.write('#include "Common.h"\n\n')
        file.write('typedef struct _tagSession SESSION;\n\n')
        file.write('class Proxy\n')
        file.write('{\n')
        file.write('public:\n')
        
        for packet in packets:
            packet_name = packet['name']
            fields = packet['fields']
            # 메서드 이름 생성
            method_name_all = f"{packet_name}_FOR_All"
            method_name_single = f"{packet_name}_FOR_SINGLE"
            # 패킷 메서드 정의
            file.write(f'    virtual void {method_name_all}(SESSION* pSession, {fields});\n')
            file.write(f'    virtual void {method_name_single}(SESSION* pSession, {fields});\n\n')
        
        file.write('};\n')

def generate_proxy_cpp_file(packets, output_filename):
    with open(output_filename, 'w', encoding='utf-8') as file:
        file.write('#include "pch.h"\n')
        file.write('#include "Proxy.h"\n\n')
        file.write('#include "Session.h"\n')
        file.write('#include "Packet.h"\n\n')
        
        for packet in packets:
            packet_name = packet['name']
            packet_define = packet['define']
            fields = packet['fields']
            # field_names는 proxy_packets에 포함되지 않으므로 새로 추출합니다
            field_names = [f.split()[-1] for f in fields.split(', ')]
            # 필드와 메서드 이름 생성
            method_name_all = f"{packet_name}_FOR_All"
            method_name_single = f"{packet_name}_FOR_SINGLE"
            
            file.write(f'void Proxy::{method_name_all}(SESSION* pSession, {fields})\n')
            file.write('{\n')
            file.write('    PACKET_HEADER header;\n')
            file.write('    CPacket Packet;\n\n')
            
            # 필드 처리
            for field_name in field_names:
                file.write(f'    Packet << {field_name};\n')
                
            file.write(f'\n    header.byCode = dfNETWORK_PACKET_CODE;\n')
            file.write(f'    header.bySize = Packet.GetDataSize();\n')
            file.write(f'    header.byType = {packet_define};\n\n')
            file.write('    BroadcastPacket(pSession, &header, &Packet);\n')
            file.write('}\n\n')
            
            file.write(f'void Proxy::{method_name_single}(SESSION* pSession, {fields})\n')
            file.write('{\n')
            file.write('    PACKET_HEADER header;\n')
            file.write('    CPacket Packet;\n\n')
            
            # 필드 처리
            for field_name in field_names:
                file.write(f'    Packet << {field_name};\n')
                
            file.write(f'\n    header.byCode = dfNETWORK_PACKET_CODE;\n')
            file.write(f'    header.bySize = Packet.GetDataSize();\n')
            file.write(f'    header.byType = {packet_define};\n\n')
            file.write('    UnicastPacket(pSession, &header, &Packet);\n')
            file.write('}\n\n')



def generate_stub_header_file(packets, output_filename):
    with open(output_filename, 'w', encoding='utf-8') as file:
        file.write('#pragma once\n\n')
        file.write('#include "Common.h"\n\n')
        file.write('typedef struct _tagSession SESSION;\n\n')
        file.write('class Stub\n')
        file.write('{\n')
        file.write('public:\n')
        
        for packet in packets:
            packet_name = packet['name']
            fields = packet['fields']
            # 메서드 이름 생성
            method_name = packet_name
            # 패킷 메서드 정의
            file.write(f'    virtual bool {method_name}(SESSION* pSession, {fields});\n')
        
        file.write('\npublic:\n')
        file.write('    bool PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket);\n')
        file.write('};\n')

def generate_stub_cpp_file(packets, output_filename):
    with open(output_filename, 'w', encoding='utf-8') as file:
        file.write('#include "pch.h"\n')
        file.write('#include "Stub.h"\n')
        file.write('#include "Packet.h"\n\n')
        
        for packet in packets:
            packet_name = packet['name']
            packet_define = packet['define']
            fields = packet['fields']
            field_names = packet['field_names'].split(', ')
            
            file.write(f'bool Stub::{packet_name}(SESSION* pSession, {fields})\n')
            file.write('{\n')
            file.write('    return true;\n')
            file.write('}\n\n')
        
        file.write('bool Stub::PacketProc(SESSION* pSession, PACKET_TYPE packetType, CPacket* pPacket)\n')
        file.write('{\n')
        file.write('    switch (packetType)\n')
        file.write('    {\n')
        
        for packet in packets:
            packet_name = packet['name']
            case_label = packet_name.replace('CS_', 'PACKET_TYPE::CS_')
            field_names = packet['field_names'].split(', ')
            file.write(f'    case {case_label}:\n')
            file.write('    {\n')
            for field_name in field_names:
                # 추출한 필드의 타입을 찾기 위해 패턴 매칭
                field_type_match = re.search(rf'(\w+)\s+{field_name}', fields)
                if field_type_match:
                    field_type = field_type_match.group(1)
                    file.write(f'        {field_type} {field_name};\n')
            file.write('\n')

            # 각 필드에 대해 개별적으로 읽어오는 코드를 작성
            for field_name in field_names:
                file.write(f'        *pPacket >> {field_name};\n')
            file.write('\n')

            file.write(f'        {packet_name}(pSession, {", ".join(field_names)});\n')
            file.write('    }\n')
            file.write('    break;\n')
        
        file.write('    default:\n')
        file.write('        break;\n')
        file.write('    }\n')
        file.write('    return true;\n')
        file.write('}\n')

def generate_enum_file(packet_enum, output_filename):
    sorted_enum = sorted(packet_enum.items(), key=lambda x: x[0])
    with open(output_filename, 'w', encoding='utf-8') as file:
        file.write('#pragma once\n\n')
        file.write('enum class PACKET_TYPE : BYTE\n')
        file.write('{\n')
        
        previous_value = -1
        for packet_id, packet_name in sorted_enum:
            if packet_id != previous_value + 1:
                file.write('\n')
            file.write(f'   {packet_name} = {packet_id},\n')
            previous_value = packet_id
        
        file.write('\n   END\n')
        file.write('};\n')

# 파일 경로를 지정하여 실행
if __name__ == "__main__":
    proxy_packets, stub_packets, packet_enum = parse_txt_file(os.getcwd() + '\RPC_Define.h')
    
    if proxy_packets:
        generate_proxy_header_file(proxy_packets, 'Proxy.h')
        generate_proxy_cpp_file(proxy_packets, 'Proxy.cpp')
        print("Proxy.h 및 Proxy.cpp 파일이 생성되었습니다.")
    
    if stub_packets:
        generate_stub_header_file(stub_packets, 'Stub.h')
        generate_stub_cpp_file(stub_packets, 'Stub.cpp')
        print("Stub.h 및 Stub.cpp 파일이 생성되었습니다.")
    
    if packet_enum:
        generate_enum_file(packet_enum, 'Common.h')
        print("Common.h 파일이 생성되었습니다.")
