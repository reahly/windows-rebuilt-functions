UINT utils::get_system_firmware_table( const DWORD firmware_table_signature, const DWORD firmware_table_id, const PVOID firmware_table_buffer, const DWORD buffer_size ) {
	ULONG result = 0, returned_size;
	const ULONG total_size = buffer_size + 0x10;

	const auto firmware_info = reinterpret_cast<uintptr_t>( malloc( total_size ) );
	if ( !firmware_info )
		return 0;

	*reinterpret_cast<uintptr_t*>( firmware_info ) = firmware_table_signature;
	*reinterpret_cast<uintptr_t*>( firmware_info + 0x4 ) = 1;
	*reinterpret_cast<uintptr_t*>( firmware_info + 0x8 ) = firmware_table_id;
	*reinterpret_cast<uintptr_t*>( firmware_info + 0xC ) = buffer_size;

	const auto status = NtQuerySystemInformation( static_cast<SYSTEM_INFORMATION_CLASS>( 0x4c ), reinterpret_cast<void*>( firmware_info ), total_size, &returned_size );
	if ( NT_SUCCESS( status ) || status == 0xC0000023 )
		result = *reinterpret_cast<DWORD*>( firmware_info + 0xC );

	if ( NT_SUCCESS( status ) && firmware_table_buffer ) {
		memcpy( firmware_table_buffer, reinterpret_cast<void*>( firmware_info + 0x10 ), *reinterpret_cast<DWORD*>( firmware_info + 0xC ) );
	}

	if ( firmware_info )
		free( reinterpret_cast<void*>( firmware_info ) );

	return result;
}

ULONG utils::get_adapters_info( PIP_ADAPTER_INFO adapter_info, const PULONG size ) {
	static auto loopback_interfaces = reinterpret_cast<int( __fastcall*)( void* )>( GetModuleHandleA( "IPHLPAPI.DLL" ) ) + 0x1038 );
	if ( !loopback_interfaces )
		return GetAdaptersInfo( adapter_info, size );
	
	void** table;
	const auto v4 = loopback_interfaces( &table );
	if ( !v4 ) {
		if ( table && adapter_info ) {
			memset( adapter_info, 0, *size );

			auto v11 = 5;
			while ( v11 ) {
				*reinterpret_cast<void**>( &adapter_info->Next ) = *table;
				*reinterpret_cast<void**>( &adapter_info->AdapterName[4] ) = table[1];
				*reinterpret_cast<void**>( &adapter_info->AdapterName[0x14] ) = table[2];
				*reinterpret_cast<void**>( &adapter_info->AdapterName[0x24] ) = table[3];
				*reinterpret_cast<void**>( &adapter_info->AdapterName[0x34] ) = table[4];
				*reinterpret_cast<void**>( &adapter_info->AdapterName[0x44] ) = table[5];
				*reinterpret_cast<void**>( &adapter_info->AdapterName[0x54] ) = table[6];
				adapter_info = reinterpret_cast<PIP_ADAPTER_INFO>( reinterpret_cast<char*>( adapter_info ) + 0x80 );

				v11--;
			}
		}
	}

	return v4;
}

BOOL utils::enum_display_devices( const LPCWSTR device_name, const DWORD device_number, const PDISPLAY_DEVICEW display_device, const DWORD flags ) {
	UNICODE_STRING dest;
	if ( device_name )
		RtlInitUnicodeString( &dest, device_name );

	static auto nt_user_enum_display_devices = reinterpret_cast<NTSTATUS( __fastcall* )( PUNICODE_STRING, DWORD, PDISPLAY_DEVICEW, DWORD )>( GetProcAddress( GetModuleHandleA( "win32u.dll" ), "NtUserEnumDisplayDevices" ) );
	if ( !nt_user_enum_display_devices )
		return EnumDisplayDevicesW( device_name, device_number, display_device, flags );

	return nt_user_enum_display_devices( device_name == nullptr ? nullptr : &dest, device_number, display_device, flags ) == 0;
}
