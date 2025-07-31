use windows::{
    core::{PCWSTR, PCSTR},
    Win32::{
        Security::{
            Cryptography::{
                NCryptOpenStorageProvider, NCryptGetProperty, NCRYPT_PROV_HANDLE,
                CryptAcquireContextW, CryptCreateHash, CryptHashData, CryptGetHashParam,
                CryptDestroyHash, CryptReleaseContext, PROV_RSA_AES, CRYPT_VERIFYCONTEXT,
                HP_HASHVAL, CALG_MD5, CALG_SHA1, CALG_SHA_256, ALG_ID,
                CryptEncodeObjectEx,
                X509_ASN_ENCODING, CNG_RSA_PUBLIC_KEY_BLOB,
                CRYPT_ENCODE_ALLOC_FLAG,
            },
            OBJECT_SECURITY_INFORMATION,
        },
        Foundation::{LocalFree, HLOCAL}
    },
};

fn bytes_to_string(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{:02x}", b)).collect()
}

unsafe fn encode_cng_key_blob(ek_pub: &[u8]) -> Option<Vec<u8>> {
    let mut encoded_ptr: *mut u8 = std::ptr::null_mut();
    let mut encoded_len: u32 = 0;

    let result = CryptEncodeObjectEx(
        X509_ASN_ENCODING,
        PCSTR(CNG_RSA_PUBLIC_KEY_BLOB.as_ptr()),
        ek_pub.as_ptr() as *const std::ffi::c_void,
        CRYPT_ENCODE_ALLOC_FLAG,
        None, // pEncodePara
        Some(&mut encoded_ptr as *mut *mut u8 as *mut std::ffi::c_void),
        &mut encoded_len,
    );

    if result.is_err() {
        return None;
    }

    let encoded = std::slice::from_raw_parts(encoded_ptr, encoded_len as usize).to_vec();
    LocalFree(Some(HLOCAL(encoded_ptr as *mut std::ffi::c_void)));
    Some(encoded)
}

fn get_ek() -> Option<Vec<u8>> {
    unsafe {
        let mut h_provider: NCRYPT_PROV_HANDLE = NCRYPT_PROV_HANDLE(0);

        let provider_name: Vec<u16> = "Microsoft Platform Crypto Provider\0".encode_utf16().collect();
        NCryptOpenStorageProvider(
            &mut h_provider,
            PCWSTR::from_raw(provider_name.as_ptr()),
            0,
        ).ok()?;

        // Query EK pub size
        let property_name: Vec<u16> = "PCP_EKPUB\0".encode_utf16().collect();
        let mut cb_result = 0u32;
        NCryptGetProperty(
            h_provider.into(),
            PCWSTR::from_raw(property_name.as_ptr()),
            None,
            &mut cb_result,
            OBJECT_SECURITY_INFORMATION(0),
        ).ok()?;

        let mut ek_pub = vec![0u8; cb_result as usize];
        NCryptGetProperty(
            h_provider.into(),
            PCWSTR::from_raw(property_name.as_ptr()),
            Some(ek_pub.as_mut_slice()),
            &mut cb_result,
            OBJECT_SECURITY_INFORMATION(0),
        ).ok()?;

        // Encode the CNG RSA public key blob into DER
        encode_cng_key_blob(&ek_pub)
    }
}

fn get_key_hash(input: &[u8], algo: ALG_ID) -> Option<String> {
    unsafe {
        let mut provider: usize = 0;
        CryptAcquireContextW(
            &mut provider,
            None,
            None,
            PROV_RSA_AES,
            CRYPT_VERIFYCONTEXT,
        ).ok()?;

        let mut hash: usize = 0;
        CryptCreateHash(
            provider,
            algo,
            0,
            0,
            &mut hash,
        ).ok()?;

        CryptHashData(
            hash,
            input,
            0,
        ).ok()?;

        // Get hash value size
        let mut hash_size = 0u32;
        CryptGetHashParam(
            hash,
            HP_HASHVAL.0,
            None,
            &mut hash_size,
            0,
        ).ok()?;

        let mut hash_val = vec![0u8; hash_size as usize];
        CryptGetHashParam(
            hash,
            HP_HASHVAL.0,
            Some(hash_val.as_mut_ptr()),
            &mut hash_size,
            0,
        ).ok()?;

        CryptDestroyHash(hash).ok();
        CryptReleaseContext(provider, 0).ok();

        Some(bytes_to_string(&hash_val))
    }
}

#[derive(serde::Serialize)]
pub struct TpmHashes {
    pub md5: Option<String>,
    pub sha1: Option<String>,
    pub sha256: Option<String>,
}

#[tauri::command]
pub fn get_tpm_hashes() -> Result<TpmHashes, String> {
    if let Some(ek_data) = get_ek() {
        Ok(TpmHashes {
            md5: get_key_hash(&ek_data, CALG_MD5),
            sha1: get_key_hash(&ek_data, CALG_SHA1),
            sha256: get_key_hash(&ek_data, CALG_SHA_256),
        })
    } else {
        Err("Failed to retrieve EK".to_string())
    }
}

pub fn get_tpm_sha256() -> Result<String, String> {
    if let Some(ek_data) = get_ek() {
        if let Some(sha256_hash) = get_key_hash(&ek_data, CALG_SHA_256) {
            Ok(sha256_hash)
        } else {
            Err("Failed to compute SHA-256 hash".to_string())
        }
    } else {
        Err("Failed to retrieve EK".to_string())
    }
}